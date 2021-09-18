#include "avatar.h" // IWYU pragma: associated

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "addiction.h"
#include "bionics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_martial_arts.h"
#include "color.h"
#include "cursesdef.h"
#include "enum_conversions.h"
#include "game_constants.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "json.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "mapsharing.h"
#include "martialarts.h"
#include "monster.h"
#include "mutation.h"
#include "name.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "pimpl.h"
#include "pldata.h"
#include "profession.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "start_location.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "units_utility.h"
#include "veh_type.h"
#include "worldfactory.h"

static const std::string flag_CHALLENGE( "CHALLENGE" );
static const std::string flag_CITY_START( "CITY_START" );
static const std::string flag_SECRET( "SECRET" );

static const std::string type_hair_style( "hair_style" );
static const std::string type_skin_tone( "skin_tone" );
static const std::string type_facial_hair( "facial_hair" );
static const std::string type_eye_color( "eye_color" );

static const flag_id json_flag_no_auto_equip( "no_auto_equip" );
static const flag_id json_flag_auto_wield( "auto_wield" );

static const json_character_flag json_flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );

static const trait_id trait_SMELLY( "SMELLY" );
static const trait_id trait_WEAKSCENT( "WEAKSCENT" );
static const trait_id trait_XS( "XS" );
static const trait_id trait_XXXL( "XXXL" );

// Responsive screen behavior for small terminal sizes
static bool isWide = false;

// Colors used in this file: (Most else defaults to c_light_gray)
#define COL_SELECT          h_light_gray   // Selected value
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
#define COL_NOTE_MINOR      c_light_gray  // Just regular note

// The point after which stats cost double
static constexpr int HIGH_STAT = 12;

// The ID of the rightmost tab
static constexpr int NEWCHAR_TAB_MAX = 7;

static int skill_increment_cost( const Character &u, const skill_id &skill );

enum struct tab_direction {
    NONE,
    FORWARD,
    BACKWARD,
    QUIT
};

enum class pool_type {
    FREEFORM = 0,
    ONE_POOL,
    MULTI_POOL,
    TRANSFER,
};

static int stat_point_pool()
{
    return 4 * 8 + get_option<int>( "INITIAL_STAT_POINTS" );
}
static int stat_points_used( const avatar &u )
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
static int trait_points_used( const avatar &u )
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
static int skill_points_used( const avatar &u )
{
    int scenario = get_scenario()->point_cost();
    int profession_points = u.prof->point_cost();
    int hobbies = 0;
    for( const profession *hobby : u.hobbies ) {
        hobbies += hobby->point_cost();
    }
    int skills = 0;
    for( const Skill &sk : Skill::skills ) {
        std::vector<int> costs = {0, 1, 1, 2, 4, 6, 9, 12, 16, 20, 25};
        skills += costs.at( u.get_skill_level( sk.ident() ) );
    }
    return scenario + profession_points + hobbies + skills;
}

static int point_pool_total()
{
    return stat_point_pool() + trait_point_pool() + skill_point_pool();
}
static int points_used_total( const avatar &u )
{
    return stat_points_used( u ) + trait_points_used( u ) + skill_points_used( u );
}

static int has_unspent_points( const avatar &u )
{
    return points_used_total( u ) < point_pool_total();
}

struct multi_pool {
    // The amount of unspent points in the pool without counting the borrowed points
    const int pure_stat_points, pure_trait_points, pure_skill_points;
    // The amount of points awailable in a pool minus the points that are borrowed
    // by lower pools plus the poits that can be borrowed from higher pools
    const int stat_points_left, trait_points_left, skill_points_left;
    explicit multi_pool( const avatar &u ):
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
            return _( "Freeform" );
    }
    return "If you see this, this is a bug";
}

static tab_direction set_points( avatar &u, pool_type & );
static tab_direction set_stats( avatar &u, pool_type );
static tab_direction set_traits( avatar &u, pool_type );
static tab_direction set_scenario( avatar &u, pool_type );
static tab_direction set_profession( avatar &u, pool_type );
static tab_direction set_hobbies( avatar &u, pool_type );
static tab_direction set_skills( avatar &u, pool_type );
static tab_direction set_description( avatar &you, bool allow_reroll, pool_type );

static cata::optional<std::string> query_for_template_name();
static void reset_scenario( avatar &u, const scenario *scen );

void Character::pick_name( bool bUseDefault )
{
    if( bUseDefault && !get_option<std::string>( "DEF_CHAR_NAME" ).empty() ) {
        name = get_option<std::string>( "DEF_CHAR_NAME" );
    } else {
        name = Name::generate( male );
    }
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
    menu.text = string_format( _( "Select a style.\n"
                                  "\n"
                                  "STR: <color_white>%d</color>, DEX: <color_white>%d</color>, "
                                  "PER: <color_white>%d</color>, INT: <color_white>%d</color>\n"
                                  "Press [<color_yellow>%s</color>] for more info.\n" ),
                               u.get_str(), u.get_dex(), u.get_per(), u.get_int(),
                               ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( 0, styles );
    menu.callback = &callback;
    menu.input_category = "MELEE_STYLE_PICKER";
    menu.additional_actions.emplace_back( "SHOW_DESCRIPTION", translation() );
    menu.desc_enabled = true;

    for( const matype_id &s : styles ) {
        const martialart &style = s.obj();
        menu.addentry_desc( style.name.translated(), style.description.translated() );
    }
    while( true ) {
        menu.query( true );
        const matype_id &selected = styles[menu.ret];
        if( query_yn( _( "Use this style?" ) ) ) {
            return selected;
        }
    }
}

void avatar::randomize( const bool random_scenario, bool play_now )
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
    // if adjusting min and max age from 16 and 55, make sure to see set_description()
    init_age = rng( 16, 55 );
    randomize_height();
    randomize_blood();
    bool cities_enabled = world_generator->active_world->WORLD_OPTIONS["CITY_SIZE"].getValue() != "0";
    if( random_scenario ) {
        std::vector<const scenario *> scenarios;
        for( const auto &scen : scenario::get_all() ) {
            if( !scen.has_flag( flag_CHALLENGE ) && !scen.scen_is_blacklisted() &&
                ( !scen.has_flag( flag_CITY_START ) || cities_enabled ) ) {
                scenarios.emplace_back( &scen );
            }
        }
        set_scenario( random_entry( scenarios ) );
    } else if( !cities_enabled ) {
        static const string_id<scenario> wilderness_only_scenario( "wilderness" );
        set_scenario( &wilderness_only_scenario.obj() );
    }

    prof = get_scenario()->weighted_random_profession();
    randomize_hobbies();
    random_start_location = true;

    str_max = rng( 6, HIGH_STAT - 2 );
    dex_max = rng( 6, HIGH_STAT - 2 );
    int_max = rng( 6, HIGH_STAT - 2 );
    per_max = rng( 6, HIGH_STAT - 2 );

    set_body();

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
                toggle_trait( rn );
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
                        toggle_trait( rn );
                        num_gtraits += mdata.points;
                    }
                    break;
                }
            /* fallthrough */
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
            /* fallthrough */
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

    randomize_cosmetic_trait( type_hair_style );
    randomize_cosmetic_trait( type_skin_tone );
    randomize_cosmetic_trait( type_eye_color );
    //arbitrary 50% chance to add beard to male characters
    if( male && one_in( 2 ) ) {
        randomize_cosmetic_trait( type_facial_hair );
    }

}

void avatar::add_profession_items()
{
    // Our profession should not be a hobby
    if( prof->is_hobby() ) {
        return;
    }

    std::list<item> prof_items = prof->items( male, get_mutations() );

    for( item &it : prof_items ) {
        if( it.has_flag( STATIC( flag_id( "WET" ) ) ) ) {
            it.active = true;
            it.item_counter = 450; // Give it some time to dry off
        }

        // TODO: debugmsg if food that isn't a seed is inedible
        if( it.has_flag( json_flag_no_auto_equip ) ) {
            it.unset_flag( json_flag_no_auto_equip );
            inv->push_back( it );
        } else if( it.has_flag( json_flag_auto_wield ) ) {
            it.unset_flag( json_flag_auto_wield );
            if( !has_wield_conflicts( it ) ) {
                wield( it );
            } else {
                inv->push_back( it );
            }
        } else if( it.is_armor() ) {
            // TODO: debugmsg if wearing fails
            wear_item( it, false, false );
        } else {
            inv->push_back( it );
        }

        if( it.is_book() ) {
            items_identified.insert( it.typeId() );
        }
    }

    recalc_sight_limits();
    calc_encumbrance();
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
    set_wielded_item( item() );

    prof = profession::generic();
    set_scenario( scenario::generic() );

    const bool interactive = type != character_type::NOW &&
                             type != character_type::FULL_RANDOM;

    int tab = 0;
    pool_type pool = pool_type::MULTI_POOL;

    switch( type ) {
        case character_type::CUSTOM:
            break;
        case character_type::RANDOM:
            //random scenario, default name if exist
            randomize( true );
            tab = NEWCHAR_TAB_MAX;
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
            // TEMPORARY until 0.F
            set_all_parts_hp_to_max();

            // We want to prevent recipes known by the template from being applied to the
            // new character. The recipe list will be rebuilt when entering the game.
            // Except if it is a character transfer template
            if( pool != pool_type::TRANSFER ) {
                learned_recipes->clear();
            }
            tab = NEWCHAR_TAB_MAX;
            break;
    }

    auto nameExists = [&]( const std::string & name ) {
        return world_generator->active_world->save_exists( save_t::from_save_id( name ) ) &&
               !query_yn( _( "A save with the name '%s' already exists in this world.\n"
                             "Saving will overwrite the already existing character.\n\n"
                             "Continue anyways?" ), name );
    };
    set_body();
    const bool allow_reroll = type == character_type::RANDOM;
    tab_direction result = tab_direction::QUIT;
    do {
        if( !interactive ) {
            // no window is created because "Play now" does not require any configuration
            if( nameExists( name ) ) {
                return false;
            }

            break;
        }

        if( pool == pool_type::TRANSFER ) {
            tab = 7;
        }

        switch( tab ) {
            case 0:
                result = set_points( *this, /*out*/ pool );
                break;
            case 1:
                result = set_scenario( *this, pool );
                break;
            case 2:
                result = set_profession( *this, pool );
                break;
            case 3:
                result = set_hobbies( *this, pool );
                break;
            case 4:
                result = set_stats( *this, pool );
                break;
            case 5:
                result = set_traits( *this, pool );
                break;
            case 6:
                result = set_skills( *this, pool );
                break;
            case 7:
                result = set_description( *this, allow_reroll, pool );
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

    if( pool == pool_type::TRANSFER ) {
        return true;
    }

    save_template( _( "Last Character" ), pool );

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

    // 2 for an average person
    float catchup_modifier = 1.0f + ( 2.0f * get_int() + get_per() ) / 24.0f;
    // 1.2 for an average person, always a bit higher than base amount
    float knowledge_modifier = 1.0f + get_int() / 40.0f;
    // Grab skills from hobbies and train
    for( const profession *profession : hobbies ) {
        for( const profession::StartingSkill &e : profession->skills() ) {
            // Train our skill
            const int skill_xp_bonus = calculate_cumulative_experience( e.second );
            get_skill_level_object( e.first ).train( skill_xp_bonus, catchup_modifier,
                    knowledge_modifier, true );
        }
    }

    // setup staring bank money
    cash = rng( -200000, 200000 );

    if( has_trait( trait_XS ) ) {
        set_stored_kcal( 10000 );
        toggle_trait( trait_XS );
    }
    if( has_trait( trait_XXXL ) ) {
        set_stored_kcal( 125000 );
        toggle_trait( trait_XXXL );
    }

    // Learn recipes
    for( const auto &e : recipe_dict ) {
        const auto &r = e.second;
        if( !r.is_practice() && !r.has_flag( flag_SECRET ) && !knows_recipe( &r ) &&
            has_recipe_requirements( r ) ) {
            learn_recipe( &r );
        }
    }
    for( const mtype_id &elem : prof->pets() ) {
        starting_pets.push_back( elem );
    }

    if( get_scenario()->vehicle() != vproto_id::NULL_ID() ) {
        starting_vehicle = get_scenario()->vehicle();
    } else {
        starting_vehicle = prof->vehicle();
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
    // Adjust current energy level to maximum
    set_power_level( get_max_power_level() );

    // Add profession proficiencies
    for( const proficiency_id &pri : prof->proficiencies() ) {
        add_proficiency( pri );
    }

    // Add hobby proficiencies
    for( const profession *profession : hobbies ) {
        for( const proficiency_id &proficiency : profession->proficiencies() ) {
            // Do not duplicate proficiencies
            if( !_proficiencies->has_learned( proficiency ) ) {
                add_proficiency( proficiency );
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
            martial_arts_data->set_style( ma_type );
        }
    }

    // Activate some mutations right from the start.
    for( const trait_id &mut : get_mutations() ) {
        const auto &branch = mut.obj();
        if( branch.starts_active ) {
            my_mutations[mut].powered = true;
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
        _( "HOBBIES" ),
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

static void draw_points( const catacurses::window &w, pool_type pool, const avatar &u,
                         int netPointCost = 0 )
{
    // Clear line (except borders)
    mvwprintz( w, point( 2, 3 ), c_black, std::string( getmaxx( w ) - 3, ' ' ) );
    std::string points_msg = pools_to_string( u, pool );
    int pMsg_length = utf8_width( remove_color_tags( points_msg ), true );
    nc_color color = c_light_gray;
    print_colored_text( w, point( 2, 3 ), color, c_light_gray, points_msg );
    if( netPointCost > 0 ) {
        mvwprintz( w, point( pMsg_length + 2, 3 ), c_red, " (-%d)", std::abs( netPointCost ) );
    } else if( netPointCost < 0 ) {
        mvwprintz( w, point( pMsg_length + 2, 3 ), c_green, " (+%d)", std::abs( netPointCost ) );
    }
}

template <class Compare>
void draw_sorting_indicator( const catacurses::window &w_sorting, const input_context &ctxt,
                             Compare sorter )
{
    const char *const sort_order = sorter.sort_by_points ? _( "points" ) : _( "name" );
    const auto sort_text = string_format(
                               _( "<color_white>Sort by:</color> %1$s "
                                  "(Press <color_light_green>%2$s</color> to change sorting.)" ),
                               sort_order, ctxt.get_desc( "SORT" ) );
    fold_and_print( w_sorting, point_zero, ( TERMX / 2 ), c_light_gray, sort_text );
}

tab_direction set_points( avatar &u, pool_type &pool )
{
    tab_direction retval = tab_direction::NONE;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( TERMY - 10, TERMX - 35, point( 31, 5 ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_POINTS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );

    const std::string point_pool = get_option<std::string>( "CHARACTER_POINT_POOLS" );

    using point_limit_tuple = std::tuple<pool_type, std::string, std::string>;
    std::vector<point_limit_tuple> opts;

    const point_limit_tuple multi_pool = std::make_tuple( pool_type::MULTI_POOL,
                                         _( "Multiple pools" ),
                                         _( "Stats, traits and skills have separate point pools.\n"
                                            "Putting stat points into traits and skills is allowed and putting trait points into skills is allowed.\n"
                                            "Scenarios and professions affect skill points." ) );

    const point_limit_tuple one_pool = std::make_tuple( pool_type::ONE_POOL, _( "Single pool" ),
                                       _( "Stats, traits and skills share a single point pool." ) );

    const point_limit_tuple freeform = std::make_tuple( pool_type::FREEFORM, _( "Freeform" ),
                                       _( "No point limits are enforced." ) );

    if( point_pool == "multi_pool" ) {
        opts = { { multi_pool } };
    } else if( point_pool == "no_freeform" ) {
        opts = { { multi_pool, one_pool } };
    } else {
        opts = { { multi_pool, one_pool, freeform } };
    }

    int highlighted = 0;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );
        draw_character_tabs( w, _( "POINTS" ) );

        const auto &cur_opt = opts[highlighted];

        draw_points( w, pool, u );

        // Clear the bottom of the screen.
        werase( w_description );

        const int opts_length = static_cast<int>( opts.size() );
        for( int i = 0; i < opts_length; i++ ) {
            nc_color color = ( pool == std::get<0>( opts[i] ) ? COL_SKILL_USED : c_light_gray );
            if( highlighted == i ) {
                color = hilite( color );
            }
            mvwprintz( w, point( 2, 5 + i ), color, std::get<1>( opts[i] ) );
        }

        fold_and_print( w_description, point_zero, getmaxx( w_description ),
                        COL_SKILL_USED, std::get<2>( cur_opt ) );

        // Helptext points tab
        if( isWide ) {
            fold_and_print( w, point( 2, TERMY - 4 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                            _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                               "Press <color_light_green>%s</color> or <color_light_green>%s</color> to select pool and "
                               "<color_light_green>%s</color> to confirm selection.\n"
                               "Press <color_light_green>%s</color> to go to the next tab or "
                               "<color_light_green>%s</color> to return to main menu." ),
                            ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
                            ctxt.get_desc( "CONFIRM" ), ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );
        } else {
            fold_and_print( w, point( 2, TERMY - 2 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                            _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                            ctxt.get_desc( "HELP_KEYBINDINGS" ) );
        }
        wnoutrefresh( w );
        wnoutrefresh( w_description );
    } );

    const int opts_length = static_cast<int>( opts.size() );
    do {
        if( highlighted < 0 ) {
            highlighted = opts_length - 1;
        } else if( highlighted >= opts_length ) {
            highlighted = 0;
        }
        ui_manager::redraw();
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
        } else if( action == "CONFIRM" ) {
            const auto &cur_opt = opts[highlighted];
            pool = std::get<0>( cur_opt );
        }
    } while( retval == tab_direction::NONE );

    return retval;
}

tab_direction set_stats( avatar &u, pool_type pool )
{
    const int max_stat_points = pool == pool_type::FREEFORM ? 20 : MAX_STAT;

    unsigned char sel = 1;
    const int iSecondColumn = std::max( 27,
                                        utf8_width( pools_to_string( u, pool ), true ) + 9 );
    input_context ctxt( "NEW_CHAR_STATS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "QUIT" );

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( 8, TERMX - iSecondColumn - 1,
                                            point( iSecondColumn, 5 ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    u.reset();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        draw_character_tabs( w, _( "STATS" ) );
        // Helptext stats tab
        fold_and_print( w, point( 2, TERMY - 5 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                        _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                           "Press <color_light_green>%s</color> / <color_light_green>%s</color> to select stat.\n"
                           "Press <color_light_green>%s</color> to increase stat or "
                           "<color_light_green>%s</color> to decrease stat.\n"
                           "Press <color_light_green>%s</color> to go to the next tab or "
                           "<color_light_green>%s</color> to return to the previous tab." ),
                        ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
                        ctxt.get_desc( "RIGHT" ), ctxt.get_desc( "LEFT" ),
                        ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );

        // This is description line, meaning its length excludes first column and border
        const std::string clear_line( getmaxx( w ) - iSecondColumn - 1, ' ' );
        mvwprintz( w, point( iSecondColumn, 3 ), c_black, clear_line );
        for( int i = 6; i < 13; i++ ) {
            mvwprintz( w, point( iSecondColumn, i ), c_black, clear_line );
        }

        draw_points( w, pool, u );

        mvwprintz( w, point( 2, 5 ), c_light_gray, _( "Strength:" ) );
        mvwprintz( w, point( 16, 5 ), c_light_gray, "%2d", u.str_max );
        mvwprintz( w, point( 2, 6 ), c_light_gray, _( "Dexterity:" ) );
        mvwprintz( w, point( 16, 6 ), c_light_gray, "%2d", u.dex_max );
        mvwprintz( w, point( 2, 7 ), c_light_gray, _( "Intelligence:" ) );
        mvwprintz( w, point( 16, 7 ), c_light_gray, "%2d", u.int_max );
        mvwprintz( w, point( 2, 8 ), c_light_gray, _( "Perception:" ) );
        mvwprintz( w, point( 16, 8 ), c_light_gray, "%2d", u.per_max );

        werase( w_description );
        switch( sel ) {
            case 1:
                mvwprintz( w, point( 2, 5 ), COL_SELECT, _( "Strength:" ) );
                mvwprintz( w, point( 16, 5 ), c_light_gray, "%2d", u.str_max );
                if( u.str_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Str further costs 2 points" ) );
                }
                u.recalc_hp();
                mvwprintz( w_description, point_zero, COL_STAT_NEUTRAL, _( "Base HP: %d" ),
                           u.get_part_hp_max( bodypart_id( "head" ) ) );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 1 ), COL_STAT_NEUTRAL, _( "Carry weight: %.1f %s" ),
                           convert_weight( u.weight_capacity() ), weight_units() );
                mvwprintz( w_description, point( 0, 2 ), COL_STAT_BONUS, _( "Bash damage bonus: %.1f" ),
                           u.bonus_damage( false ) );
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, c_green,
                                _( "Strength also makes you more resistant to many diseases and poisons, and makes actions which require brute force more effective." ) );
                break;

            case 2:
                mvwprintz( w, point( 2, 6 ), COL_SELECT, _( "Dexterity:" ) );
                mvwprintz( w, point( 16, 6 ), c_light_gray, "%2d", u.dex_max );
                if( u.dex_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Dex further costs 2 points" ) );
                }
                mvwprintz( w_description, point_zero, COL_STAT_BONUS, _( "Melee to-hit bonus: +%.2f" ),
                           u.get_melee_hit_base() );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 1 ), COL_STAT_BONUS,
                           _( "Throwing penalty per target's dodge: +%d" ),
                           u.throw_dispersion_per_dodge( false ) );
                if( u.ranged_dex_mod() != 0 ) {
                    mvwprintz( w_description, point( 0, 2 ), COL_STAT_PENALTY, _( "Ranged penalty: -%d" ),
                               std::abs( u.ranged_dex_mod() ) );
                }
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, c_green,
                                _( "Dexterity also enhances many actions which require finesse." ) );
                break;

            case 3: {
                mvwprintz( w, point( 2, 7 ), COL_SELECT, _( "Intelligence:" ) );
                mvwprintz( w, point( 16, 7 ), c_light_gray, "%2d", u.int_max );
                if( u.int_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Int further costs 2 points" ) );
                }
                const int read_spd = u.read_speed( false );
                mvwprintz( w_description, point_zero, ( read_spd == 100 ? COL_STAT_NEUTRAL :
                                                        ( read_spd < 100 ? COL_STAT_BONUS : COL_STAT_PENALTY ) ),
                           _( "Read times: %d%%" ), read_spd );
                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_description, point( 0, 2 ), COL_STAT_BONUS, _( "Crafting bonus: %2d%%" ),
                           u.get_int() );
                fold_and_print( w_description, point( 0, 4 ), getmaxx( w_description ) - 1, c_green,
                                _( "Intelligence is also used when crafting, installing bionics, and interacting with NPCs." ) );
            }
            break;

            case 4:
                mvwprintz( w, point( 2, 8 ), COL_SELECT, _( "Perception:" ) );
                mvwprintz( w, point( 16, 8 ), c_light_gray, "%2d", u.per_max );
                if( u.per_max >= HIGH_STAT ) {
                    mvwprintz( w, point( iSecondColumn, 3 ), c_light_red,
                               _( "Increasing Per further costs 2 points" ) );
                }
                if( u.ranged_per_mod() > 0 ) {
                    mvwprintz( w_description, point_zero, COL_STAT_PENALTY, _( "Aiming penalty: -%d" ),
                               u.ranged_per_mod() );
                }
                fold_and_print( w_description, point( 0, 2 ), getmaxx( w_description ) - 1, c_green,
                                _( "Perception is also used for detecting traps and other things of interest." ) );
                break;
        }

        wnoutrefresh( w );
        wnoutrefresh( w_description );
    } );

    do {
        ui_manager::redraw();
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
                u.str_max--;
            } else if( sel == 2 && u.dex_max > 4 ) {
                u.dex_max--;
            } else if( sel == 3 && u.int_max > 4 ) {
                u.int_max--;
            } else if( sel == 4 && u.per_max > 4 ) {
                u.per_max--;
            }
        } else if( action == "RIGHT" ) {
            if( sel == 1 && u.str_max < max_stat_points ) {
                u.str_max++;
            } else if( sel == 2 && u.dex_max < max_stat_points ) {
                u.dex_max++;
            } else if( sel == 3 && u.int_max < max_stat_points ) {
                u.int_max++;
            } else if( sel == 4 && u.per_max < max_stat_points ) {
                u.per_max++;
            }
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            return tab_direction::FORWARD;
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

static struct {
    /** @related player */
    bool operator()( const trait_id *a, const trait_id *b ) {
        return std::abs( a->obj().points ) > std::abs( b->obj().points );
    }
} traits_sorter;

tab_direction set_traits( avatar &u, pool_type pool )
{
    const int max_trait_points = get_option<int>( "MAX_TRAIT_POINTS" );

    // Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
    int num_good = 0;
    int num_bad = 0;
    // 0 -> traits that take points ( positive traits )
    // 1 -> traits that give points ( negative traits )
    // 2 -> neutral traits ( facial hair, skin color, etc )
    std::vector<trait_id> vStartingTraits[3];

    for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
        // Don't list blacklisted traits
        if( mutation_branch::trait_is_blacklisted( traits_iter.id ) ) {
            continue;
        }

        const std::set<trait_id> scentraits = get_scenario()->get_locked_traits();
        const bool is_scentrait = scentraits.find( traits_iter.id ) != scentraits.end();

        // Always show profession locked traits, regardless of if they are forbidden
        const std::vector<trait_id> proftraits = u.prof->get_locked_traits();
        const bool is_proftrait = std::find( proftraits.begin(), proftraits.end(),
                                             traits_iter.id ) != proftraits.end();

        // We show all starting traits, even if we can't pick them, to keep the interface consistent.
        if( traits_iter.startingtrait || get_scenario()->traitquery( traits_iter.id ) || is_proftrait ) {
            if( traits_iter.points > 0 ) {
                vStartingTraits[0].push_back( traits_iter.id );

                if( is_proftrait || is_scentrait ) {
                    continue;
                }

                if( u.has_trait( traits_iter.id ) ) {
                    num_good += traits_iter.points;
                }
            } else if( traits_iter.points < 0 ) {
                vStartingTraits[1].push_back( traits_iter.id );

                if( is_proftrait || is_scentrait ) {
                    continue;
                }

                if( u.has_trait( traits_iter.id ) ) {
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
        std::sort( vStartingTrait.begin(), vStartingTrait.end(), trait_display_nocolor_sort );
    }

    int iCurWorkingPage = 0;
    int iStartPos[3] = { 0, 0, 0 };
    int iCurrentLine[3] = { 0, 0, 0 };
    size_t traits_size[3];
    bool recalc_traits = false;
    std::vector<const trait_id *> sorted_traits[3];
    std::string filterstring;

    for( int i = 0; i < 3; i++ ) {
        const size_t size = vStartingTraits[i].size();
        traits_size[i] = size;
        sorted_traits[i].reserve( size );
        for( size_t j = 0; j < size; j++ ) {
            sorted_traits[i].emplace_back( &vStartingTraits[i][j] );
        }
    }

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
    catacurses::window w_description;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( 3, TERMX - 2, point( 1, TERMY - 4 ) );
        ui.position_from_window( w );
        page_width = std::min( ( TERMX - 4 ) / used_pages, 38 );
        iContentHeight = TERMY - 9;

        pos_calc();
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_TRAITS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "QUIT" );

    bool unsorted = true;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        werase( w_description );

        draw_character_tabs( w, _( "TRAITS" ) );

        const std::string filter_indicator = filterstring.empty() ?
                                             _( "no filter" ) : filterstring;
        const std::string sortstring = string_format( "[%1$s] %2$s: %3$s", ctxt.get_desc( "SORT" ),
                                       _( "sort" ), unsorted ? _( "unsorted" ) : _( "points" ) );

        mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );
        mvwprintz( w, point( getmaxx( w ) - sortstring.size() - 4, getmaxy( w ) - 1 ),
                   c_light_gray, "<%s>", sortstring );

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
            nc_color hi_off = hilite( col_off_act );

            int &start = iStartPos[iCurrentPage];
            int current = iCurrentLine[iCurrentPage];
            calcStartPos( start, current, iContentHeight, traits_size[iCurrentPage] );
            int end = start + static_cast<int>( std::min( traits_size[iCurrentPage], iContentHeight ) );

            for( int i = start; i < end; i++ ) {
                const trait_id &cur_trait = *sorted_traits[iCurrentPage][i];
                const mutation_branch &mdata = cur_trait.obj();
                if( current == i && iCurrentPage == iCurWorkingPage ) {
                    int points = mdata.points;
                    bool negativeTrait = points < 0;
                    if( negativeTrait ) {
                        points *= -1;
                    }
                    mvwprintz( w, point( full_string_length + 3, 3 ), col_tr,
                               n_gettext( "%s %s %d point", "%s %s %d points", points ),
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

                int cur_line_y = 5 + i - start;
                int cur_line_x = 2 + iCurrentPage * page_width;
                mvwprintz( w, point( cur_line_x, cur_line_y ), cLine, utf8_truncate( mdata.name(),
                           page_width - 2 ) );
            }

            draw_scrollbar( w, iCurrentLine[iCurrentPage], iContentHeight, traits_size[iCurrentPage],
                            point( page_width * iCurrentPage, 5 ) );
        }

        wnoutrefresh( w );
        wnoutrefresh( w_description );
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
                        return !lcmatch( trait->obj().name(), filterstring );
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

            if( !unsorted ) {
                std::stable_sort( sorted_traits[0].begin(), sorted_traits[0].end(), traits_sorter );
                std::stable_sort( sorted_traits[1].begin(), sorted_traits[1].end(), traits_sorter );
            }

            // Select the current page, if not empty
            // There should always be atleast one not empty page
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
        if( action == "LEFT" ) {
            iCurWorkingPage = next_avail_page( true );
        } else if( action == "RIGHT" ) {
            iCurWorkingPage = next_avail_page( false );
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
        } else if( action == "PAGE_DOWN" ) {
            if( static_cast<size_t>( iCurrentLine[iCurWorkingPage] ) == traits_size[iCurWorkingPage] - 1 ) {
                iCurrentLine[iCurWorkingPage] = 0;
            } else if( static_cast<size_t>( iCurrentLine[iCurWorkingPage] + 10 ) >=
                       traits_size[iCurWorkingPage] ) {
                iCurrentLine[iCurWorkingPage] = traits_size[iCurWorkingPage] - 1;
            } else {
                iCurrentLine[iCurWorkingPage] += +10;
            }
        } else if( action == "PAGE_UP" ) {
            if( iCurrentLine[iCurWorkingPage] == 0 ) {
                iCurrentLine[iCurWorkingPage] = traits_size[iCurWorkingPage] - 1;
            } else if( static_cast<size_t>( iCurrentLine[iCurWorkingPage] - 10 ) >=
                       traits_size[iCurWorkingPage] ) {
                iCurrentLine[iCurWorkingPage] = 0;
            } else {
                iCurrentLine[iCurWorkingPage] += -10;
            }
        } else if( action == "CONFIRM" ) {
            int inc_type = 0;
            const trait_id cur_trait = *sorted_traits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
            const mutation_branch &mdata = cur_trait.obj();

            // Look through the profession bionics, and see if any of them conflict with this trait
            std::vector<bionic_id> cbms_blocking_trait = bionics_cancelling_trait( u.prof->CBMs(), cur_trait );
            const std::unordered_set<trait_id> conflicting_traits = u.get_conflicting_traits( cur_trait );

            if( u.has_trait( cur_trait ) ) {

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
                        popup( _( "Your hobby of %s prevents you from removing this trait." ),
                               hobbies->gender_appropriate_name( u.male ) );
                    }
                }
            } else if( !conflicting_traits.empty() ) {
                std::vector<std::string> conflict_names;
                conflict_names.reserve( conflicting_traits.size() );
                for( const trait_id &trait : conflicting_traits ) {
                    conflict_names.emplace_back( trait->name() );
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
                inc_type = 1;
            }

            //inc_type is either -1 or 1, so we can just multiply by it to invert
            if( inc_type != 0 ) {
                u.toggle_trait( cur_trait );
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
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        } else if( action == "SORT" ) {
            unsorted = !unsorted;
            recalc_traits = true;
        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Search:" ) )
            .width( 10 )
            .description( _( "Search by trait name." ) )
            .edit( filterstring );
            recalc_traits = true;
        }
    } while( true );
}

static struct {
    bool sort_by_points = true;
    bool male = false;
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
            return localized_compare( a->gender_appropriate_name( male ),
                                      b->gender_appropriate_name( male ) );
        }
    }
} profession_sorter;

/** Handle the profession tab of the character generation menu */
tab_direction set_profession( avatar &u, pool_type pool )
{
    int cur_id = 0;
    tab_direction retval = tab_direction::NONE;
    int desc_offset = 0;
    int iContentHeight = 0;
    int iStartPos = 0;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    catacurses::window w_sorting;
    catacurses::window w_genderswap;
    catacurses::window w_items;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - 10;
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( 4, TERMX - 2, point( 1, TERMY - 5 ) );
        w_sorting = catacurses::newwin( 1, 55, point( TERMX / 2, 5 ) );
        w_genderswap = catacurses::newwin( 1, 55, point( TERMX / 2, 6 ) );
        w_items = catacurses::newwin( iContentHeight - 3, 55, point( TERMX / 2, 8 ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_PROFESSIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "RANDOMIZE" );

    bool recalc_profs = true;
    int profs_length = 0;
    std::string filterstring;
    std::vector<string_id<profession>> sorted_profs;

    int iheight = 0;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        draw_character_tabs( w, _( "PROFESSION" ) );

        // Draw filter indicator
        for( int i = 1; i < getmaxx( w ) - 1; i++ ) {
            mvwputch( w, point( i, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXOX );
        }
        const auto filter_indicator = filterstring.empty() ? _( "no filter" )
                                      : filterstring;
        mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_profs.size();

        werase( w_description );
        if( cur_id_is_valid ) {
            int netPointCost = sorted_profs[cur_id]->point_cost() - u.prof->point_cost();
            bool can_pick = sorted_profs[cur_id]->can_pick( u, skill_points_left( u, pool ) );
            const std::string clear_line( getmaxx( w ) - 2, ' ' );

            // Clear the bottom of the screen and header.
            mvwprintz( w, point( 1, 3 ), c_light_gray, clear_line );

            int pointsForProf = sorted_profs[cur_id]->point_cost();
            bool negativeProf = pointsForProf < 0;
            if( negativeProf ) {
                pointsForProf *= -1;
            }
            // Draw header.
            draw_points( w, pool, u, netPointCost );
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
            mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick ? c_green : c_light_red, prof_msg_temp,
                       sorted_profs[cur_id]->gender_appropriate_name( u.male ),
                       pointsForProf );

            fold_and_print( w_description, point_zero, TERMX - 2, c_green,
                            sorted_profs[cur_id]->description( u.male ) );
        }

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
                col = ( cur_id_is_valid && sorted_profs[i] == sorted_profs[cur_id] ? COL_SELECT : c_light_gray );
            } else {
                col = ( cur_id_is_valid &&
                        sorted_profs[i] == sorted_profs[cur_id] ? hilite( COL_SKILL_USED ) : COL_SKILL_USED );
            }
            mvwprintz( w, point( 2, 5 + i - iStartPos ), col,
                       sorted_profs[i]->gender_appropriate_name( u.male ) );
        }
        //Clear rest of space in case stuff got filtered out
        for( ; i < iStartPos + iContentHeight; ++i ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line
        }

        werase( w_items );
        werase( w_sorting );
        werase( w_genderswap );
        if( cur_id_is_valid ) {
            std::string buffer;
            // Profession addictions
            const auto prof_addictions = sorted_profs[cur_id]->addictions();
            buffer += colorize( _( "Addictions:" ), c_light_blue ) + "\n";
            if( prof_addictions.empty() ) {
                buffer += pgettext( "set_profession_addictions", "None" ) + std::string( "\n" );
            } else {
                for( const addiction &a : prof_addictions ) {
                    const char *format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
                    buffer += string_format( format, addiction_name( a ), a.intensity ) + "\n";
                }
            }

            // Profession traits
            const auto prof_traits = sorted_profs[cur_id]->get_locked_traits();
            buffer += colorize( _( "Profession traits:" ), c_light_blue ) + "\n";
            if( prof_traits.empty() ) {
                buffer += pgettext( "set_profession_trait", "None" ) + std::string( "\n" );
            } else {
                for( const auto &t : prof_traits ) {
                    buffer += mutation_branch::get_name( t ) + "\n";
                }
            }

            // Profession skills
            const auto prof_skills = sorted_profs[cur_id]->skills();
            buffer += colorize( _( "Profession skills:" ), c_light_blue ) + "\n";
            if( prof_skills.empty() ) {
                buffer += pgettext( "set_profession_skill", "None" ) + std::string( "\n" );
            } else {
                for( const auto &sl : prof_skills ) {
                    const char *format = pgettext( "set_profession_skill", "%1$s (%2$d)" );
                    buffer += string_format( format, sl.first.obj().name(), sl.second ) + "\n";
                }
            }

            // Profession items
            const auto prof_items = sorted_profs[cur_id]->items( u.male, u.get_mutations() );
            buffer += colorize( _( "Profession items:" ), c_light_blue ) + "\n";
            if( prof_items.empty() ) {
                buffer += pgettext( "set_profession_item", "None" ) + std::string( "\n" );
            } else {
                // TODO: If the item group is randomized *at all*, these will be different each time
                // and it won't match what you actually start with
                // TODO: Put like items together like the inventory does, so we don't have to scroll
                // through a list of a dozen forks.
                std::string buffer_wielded;
                std::string buffer_worn;
                std::string buffer_inventory;
                for( const auto &it : prof_items ) {
                    if( it.has_flag( json_flag_no_auto_equip ) ) { // NOLINT(bugprone-branch-clone)
                        buffer_inventory += it.display_name() + "\n";
                    } else if( it.has_flag( json_flag_auto_wield ) ) {
                        buffer_wielded += it.display_name() + "\n";
                    } else if( it.is_armor() ) {
                        buffer_worn += it.display_name() + "\n";
                    } else {
                        buffer_inventory += it.display_name() + "\n";
                    }
                }
                buffer += colorize( _( "Wielded:" ), c_cyan ) + "\n";
                buffer += !buffer_wielded.empty() ? buffer_wielded : pgettext( "set_profession_item_wielded",
                          "None\n" );
                buffer += colorize( _( "Worn:" ), c_cyan ) + "\n";
                buffer += !buffer_worn.empty() ? buffer_worn : pgettext( "set_profession_item_worn", "None\n" );
                buffer += colorize( _( "Inventory:" ), c_cyan ) + "\n";
                buffer += !buffer_inventory.empty() ? buffer_inventory : pgettext( "set_profession_item_inventory",
                          "None\n" );
            }

            // Profession bionics, active bionics shown first
            auto prof_CBMs = sorted_profs[cur_id]->CBMs();
            std::sort( begin( prof_CBMs ), end( prof_CBMs ), []( const bionic_id & a, const bionic_id & b ) {
                return a->activated && !b->activated;
            } );
            buffer += colorize( _( "Profession bionics:" ), c_light_blue ) + "\n";
            if( prof_CBMs.empty() ) {
                buffer += pgettext( "set_profession_bionic", "None" ) + std::string( "\n" );
            } else {
                for( const auto &b : prof_CBMs ) {
                    const auto &cbm = b.obj();
                    if( cbm.activated && cbm.has_flag( json_flag_BIONIC_TOGGLED ) ) {
                        buffer += string_format( _( "%s (toggled)" ), cbm.name ) + "\n";
                    } else if( cbm.activated ) {
                        buffer += string_format( _( "%s (activated)" ), cbm.name ) + "\n";
                    } else {
                        buffer += cbm.name + "\n";
                    }
                }
            }
            // Proficiencies
            const std::string newline = "\n";
            std::vector<proficiency_id> prof_proficiencies = sorted_profs[cur_id]->proficiencies();
            buffer += colorize( _( "Profession proficiencies:" ), c_light_blue ) + newline;
            if( prof_proficiencies.empty() ) {
                buffer += pgettext( "Profession has no proficiencies", "None" ) + newline;
            } else {
                for( const proficiency_id &prof : prof_proficiencies ) {
                    buffer += prof->name() + newline;
                }
            }
            // Profession pet
            buffer += colorize( _( "Pets:" ), c_light_blue ) + "\n";
            if( sorted_profs[cur_id]->pets().empty() ) {
                buffer += pgettext( "set_profession_pets", "None" ) + std::string( "\n" );
            } else {
                for( const auto &elem : sorted_profs[cur_id]->pets() ) {
                    monster mon( elem );
                    buffer += mon.get_name() + "\n";
                }
            }
            // Profession vehicle
            buffer += colorize( _( "Vehicle:" ), c_light_blue ) + "\n";
            if( sorted_profs[cur_id]->vehicle() ) {
                vproto_id veh_id = sorted_profs[cur_id]->vehicle();
                buffer += veh_id->name.translated();
            } else {
                buffer += pgettext( "set_profession_vehicle", "None" ) + std::string( "\n" );
            }
            // Profession spells
            if( !sorted_profs[cur_id]->spells().empty() ) {
                buffer += colorize( _( "Spells:" ), c_light_blue ) + "\n";
                for( const std::pair<spell_id, int> spell_pair : sorted_profs[cur_id]->spells() ) {
                    buffer += string_format( _( "%s level %d" ), spell_pair.first->name, spell_pair.second ) + "\n";
                }
            }

            const auto scroll_msg = string_format(
                                        _( "Press <color_light_green>%1$s</color> or <color_light_green>%2$s</color> to scroll." ),
                                        ctxt.get_desc( "LEFT" ),
                                        ctxt.get_desc( "RIGHT" ) );
            iheight = print_scrollable( w_items, desc_offset, buffer, c_light_gray, scroll_msg );

            draw_sorting_indicator( w_sorting, ctxt, profession_sorter );

            const char *g_switch_msg = u.male ?
                                       //~ Gender switch message. 1s - change key name, 2s - profession name.
                                       _( "Press <color_light_green>%1$s</color> to switch "
                                          "to <color_magenta>%2$s</color> (<color_pink>female</color>)." ) :
                                       //~ Gender switch message. 1s - change key name, 2s - profession name.
                                       _( "Press <color_light_green>%1$s</color> to switch "
                                          "to <color_magenta>%2$s</color> (<color_light_cyan>male</color>)." );
            fold_and_print( w_genderswap, point_zero, ( TERMX / 2 ), c_light_gray, g_switch_msg,
                            ctxt.get_desc( "CHANGE_GENDER" ),
                            sorted_profs[cur_id]->gender_appropriate_name( !u.male ) );
        }

        draw_scrollbar( w, cur_id, iContentHeight, profs_length, point( 0, 5 ) );

        wnoutrefresh( w );
        wnoutrefresh( w_description );
        wnoutrefresh( w_items );
        wnoutrefresh( w_genderswap );
        wnoutrefresh( w_sorting );
    } );

    do {
        if( recalc_profs ) {
            sorted_profs = get_scenario()->permitted_professions();

            // Remove all hobbies and filter our list
            const auto new_end = std::remove_if( sorted_profs.begin(),
            sorted_profs.end(), [&]( const string_id<profession> &arg ) {
                return !lcmatch( arg->gender_appropriate_name( u.male ), filterstring );
            } );
            sorted_profs.erase( new_end, sorted_profs.end() );

            if( sorted_profs.empty() ) {
                popup( _( "Nothing found." ) ); // another case of black box in tiles
                filterstring.clear();
                continue;
            }
            profs_length = static_cast<int>( sorted_profs.size() );

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

            recalc_profs = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int recmax = profs_length;
        const int scroll_rate = recmax > 20 ? 10 : 2;

        if( action == "DOWN" ) {
            cur_id++;
            if( cur_id > recmax - 1 ) {
                cur_id = 0;
            }
            desc_offset = 0;
        } else if( action == "UP" ) {
            cur_id--;
            if( cur_id < 0 ) {
                cur_id = profs_length - 1;
            }
            desc_offset = 0;
        } else if( action == "PAGE_DOWN" ) {
            if( cur_id == recmax - 1 ) {
                cur_id = 0;
            } else if( cur_id + scroll_rate >= recmax ) {
                cur_id = recmax - 1;
            } else {
                cur_id += +scroll_rate;
            }
            desc_offset = 0;
        } else if( action == "PAGE_UP" ) {
            if( cur_id == 0 ) {
                cur_id = recmax - 1;
            } else if( cur_id <= scroll_rate ) {
                cur_id = 0;
            } else {
                cur_id += -scroll_rate;
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
                u.toggle_trait( old_trait );
            }

            u.prof = &sorted_profs[cur_id].obj();

            // Remove pre-selected traits that conflict
            // with the new profession's traits
            for( const trait_id &new_trait : u.prof->get_locked_traits() ) {
                if( u.has_conflicting_trait( new_trait ) ) {
                    for( const trait_id &suspect_trait : u.get_mutations() ) {
                        if( are_conflicting_traits( new_trait, suspect_trait ) ) {
                            u.toggle_trait( suspect_trait );
                            popup( _( "Your trait %1$s has been removed since it conflicts with the %2$s's %3$s trait." ),
                                   suspect_trait->name(), u.prof->gender_appropriate_name( u.male ), new_trait->name() );
                        }
                    }
                }
            }
            // Add traits for the new profession (and perhaps scenario, if, for example,
            // both the scenario and old profession require the same trait)
            u.add_traits();
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
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, profs_length - 1 );
        }

    } while( retval == tab_direction::NONE );

    return retval;
}

/** Handle the hobbies tab of the character generation menu */
tab_direction set_hobbies( avatar &u, pool_type pool )
{
    int cur_id = 0;
    tab_direction retval = tab_direction::NONE;
    int desc_offset = 0;
    int iContentHeight = 0;
    int iStartPos = 0;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    catacurses::window w_sorting;
    catacurses::window w_genderswap;
    catacurses::window w_items;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - 10;
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( 4, TERMX - 2, point( 1, TERMY - 5 ) );
        w_sorting = catacurses::newwin( 1, 55, point( TERMX / 2, 5 ) );
        w_genderswap = catacurses::newwin( 1, 55, point( TERMX / 2, 6 ) );
        w_items = catacurses::newwin( iContentHeight - 3, 55, point( TERMX / 2, 8 ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_PROFESSIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "RANDOMIZE" );

    bool recalc_profs = true;
    int profs_length = 0;
    std::string filterstring;
    std::vector<string_id<profession>> sorted_profs;

    int iheight = 0;
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        draw_character_tabs( w, _( "HOBBIES" ) );

        // Draw filter indicator
        for( int i = 1; i < getmaxx( w ) - 1; i++ ) {
            mvwputch( w, point( i, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXOX );
        }
        const auto filter_indicator = filterstring.empty() ? _( "no filter" )
                                      : filterstring;
        mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_profs.size();

        werase( w_description );
        if( cur_id_is_valid ) {
            int netPointCost = sorted_profs[cur_id]->point_cost() - u.prof->point_cost();
            bool can_pick = sorted_profs[cur_id]->can_pick( u, skill_points_left( u, pool ) );
            const std::string clear_line( getmaxx( w ) - 2, ' ' );

            // Clear the bottom of the screen and header.
            mvwprintz( w, point( 1, 3 ), c_light_gray, clear_line );

            int pointsForProf = sorted_profs[cur_id]->point_cost();
            bool negativeProf = pointsForProf < 0;
            if( negativeProf ) {
                pointsForProf *= -1;
            }
            // Draw header.
            draw_points( w, pool, u, netPointCost );
            const char *prof_msg_temp;
            if( negativeProf ) {
                //~ 1s - profession name, 2d - current character points.
                prof_msg_temp = n_gettext( "Hobby %1$s earns %2$d point",
                                           "Hobby %1$s earns %2$d points",
                                           pointsForProf );
            } else {
                //~ 1s - profession name, 2d - current character points.
                prof_msg_temp = n_gettext( "Hobby %1$s costs %2$d point",
                                           "Hobby %1$s costs %2$d points",
                                           pointsForProf );
            }

            int pMsg_length = utf8_width( remove_color_tags( pools_to_string( u, pool ) ) );
            mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick ? c_green : c_light_red, prof_msg_temp,
                       sorted_profs[cur_id]->gender_appropriate_name( u.male ),
                       pointsForProf );

            fold_and_print( w_description, point_zero, TERMX - 2, c_green,
                            sorted_profs[cur_id]->description( u.male ) );
        }

        //Draw options
        calcStartPos( iStartPos, cur_id, iContentHeight, profs_length );
        const int end_pos = iStartPos + ( ( iContentHeight > profs_length ) ?
                                          profs_length : iContentHeight );
        int i;
        for( i = iStartPos; i < end_pos; i++ ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line

            nc_color col;
            if( u.hobbies.count( &sorted_profs[i].obj() ) != 0 ) {
                col = ( cur_id_is_valid &&
                        sorted_profs[i] == sorted_profs[cur_id] ? hilite( COL_SKILL_USED ) : COL_SKILL_USED );
            } else {
                col = ( cur_id_is_valid && sorted_profs[i] == sorted_profs[cur_id] ? COL_SELECT : c_light_gray );
            }

            mvwprintz( w, point( 2, 5 + i - iStartPos ), col,
                       sorted_profs[i]->gender_appropriate_name( u.male ) );
        }
        //Clear rest of space in case stuff got filtered out
        for( ; i < iStartPos + iContentHeight; ++i ) {
            mvwprintz( w, point( 2, 5 + i - iStartPos ), c_light_gray,
                       "                                             " ); // Clear the line
        }

        werase( w_items );
        werase( w_sorting );
        werase( w_genderswap );
        if( cur_id_is_valid ) {
            std::string buffer;
            // Profession addictions
            const auto prof_addictions = sorted_profs[cur_id]->addictions();
            buffer += colorize( _( "Addictions:" ), c_light_blue ) + "\n";
            if( prof_addictions.empty() ) {
                buffer += pgettext( "set_profession_addictions", "None" ) + std::string( "\n" );
            } else {
                for( const addiction &a : prof_addictions ) {
                    const char *format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
                    buffer += string_format( format, addiction_name( a ), a.intensity ) + "\n";
                }
            }

            // Profession traits
            const auto prof_traits = sorted_profs[cur_id]->get_locked_traits();
            buffer += colorize( _( "Hobby traits:" ), c_light_blue ) + "\n";
            if( prof_traits.empty() ) {
                buffer += pgettext( "set_profession_trait", "None" ) + std::string( "\n" );
            } else {
                for( const auto &t : prof_traits ) {
                    buffer += mutation_branch::get_name( t ) + "\n";
                }
            }

            // Profession skills
            const auto prof_skills = sorted_profs[cur_id]->skills();
            buffer += colorize( _( "Hobby skill experience:" ), c_light_blue ) + "\n";
            if( prof_skills.empty() ) {
                buffer += pgettext( "set_profession_skill", "None" ) + std::string( "\n" );
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
                    buffer += string_format( "%s (%s)", skill.name(), skill_degree ) + "\n";
                }
            }

            // Proficiencies
            const std::string newline = "\n";
            std::vector<proficiency_id> prof_proficiencies = sorted_profs[cur_id]->proficiencies();
            buffer += colorize( _( "Hobby proficiencies:" ), c_light_blue ) + newline;
            if( prof_proficiencies.empty() ) {
                buffer += pgettext( "Hobby has no proficiencies", "None" ) + newline;
            } else {
                for( const proficiency_id &prof : prof_proficiencies ) {
                    buffer += prof->name() + newline;
                }
            }

            // Profession spells
            if( !sorted_profs[cur_id]->spells().empty() ) {
                buffer += colorize( _( "Spells:" ), c_light_blue ) + "\n";
                for( const std::pair<spell_id, int> spell_pair : sorted_profs[cur_id]->spells() ) {
                    buffer += string_format( _( "%s level %d" ), spell_pair.first->name, spell_pair.second ) + "\n";
                }
            }

            const auto scroll_msg = string_format(
                                        _( "Press <color_light_green>%1$s</color> or <color_light_green>%2$s</color> to scroll." ),
                                        ctxt.get_desc( "LEFT" ),
                                        ctxt.get_desc( "RIGHT" ) );
            iheight = print_scrollable( w_items, desc_offset, buffer, c_light_gray, scroll_msg );

            draw_sorting_indicator( w_sorting, ctxt, profession_sorter );

            const char *g_switch_msg = u.male ?
                                       //~ Gender switch message. 1s - change key name, 2s - profession name.
                                       _( "Press <color_light_green>%1$s</color> to switch "
                                          "to <color_magenta>%2$s</color> (<color_pink>female</color>)." ) :
                                       //~ Gender switch message. 1s - change key name, 2s - profession name.
                                       _( "Press <color_light_green>%1$s</color> to switch "
                                          "to <color_magenta>%2$s</color> (<color_light_cyan>male</color>)." );
            fold_and_print( w_genderswap, point_zero, ( TERMX / 2 ), c_light_gray, g_switch_msg,
                            ctxt.get_desc( "CHANGE_GENDER" ),
                            sorted_profs[cur_id]->gender_appropriate_name( !u.male ) );
        }

        draw_scrollbar( w, cur_id, iContentHeight, profs_length, point( 0, 5 ) );

        wnoutrefresh( w );
        wnoutrefresh( w_description );
        wnoutrefresh( w_items );
        wnoutrefresh( w_genderswap );
        wnoutrefresh( w_sorting );
    } );

    do {
        if( recalc_profs ) {
            sorted_profs = get_scenario()->permitted_professions();
            sorted_profs = profession::get_all_hobbies();

            // Remove items based on filter
            const auto new_end = std::remove_if( sorted_profs.begin(),
            sorted_profs.end(), [&]( const string_id<profession> &arg ) {
                return !lcmatch( arg->gender_appropriate_name( u.male ), filterstring );
            } );
            sorted_profs.erase( new_end, sorted_profs.end() );

            if( sorted_profs.empty() ) {
                popup( _( "Nothing found." ) );
                filterstring.clear();
                continue;
            }
            profs_length = static_cast<int>( sorted_profs.size() );

            // Sort professions by points.
            // profession_display_sort() keeps "unemployed" at the top.
            profession_sorter.male = u.male;
            std::stable_sort( sorted_profs.begin(), sorted_profs.end(), profession_sorter );

            cur_id = 0;
            recalc_profs = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int recmax = profs_length;
        const int scroll_rate = recmax > 20 ? 10 : 2;

        if( action == "DOWN" ) {
            cur_id++;
            if( cur_id > recmax - 1 ) {
                cur_id = 0;
            }
            desc_offset = 0;
        } else if( action == "UP" ) {
            cur_id--;
            if( cur_id < 0 ) {
                cur_id = profs_length - 1;
            }
            desc_offset = 0;
        } else if( action == "PAGE_DOWN" ) {
            if( cur_id == recmax - 1 ) {
                cur_id = 0;
            } else if( cur_id + scroll_rate >= recmax ) {
                cur_id = recmax - 1;
            } else {
                cur_id += +scroll_rate;
            }
            desc_offset = 0;
        } else if( action == "PAGE_UP" ) {
            if( cur_id == 0 ) {
                cur_id = recmax - 1;
            } else if( cur_id <= scroll_rate ) {
                cur_id = 0;
            } else {
                cur_id += -scroll_rate;
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
            // Do not allow selection of hobby if there's a trait conflict
            const profession *prof = &sorted_profs[cur_id].obj();
            bool conflict_found = false;
            for( const trait_id &new_trait : prof->get_locked_traits() ) {
                if( u.has_conflicting_trait( new_trait ) ) {
                    for( const profession *hobby : u.hobbies ) {
                        for( const trait_id &suspect_trait : hobby->get_locked_traits() ) {
                            if( are_conflicting_traits( new_trait, suspect_trait ) ) {
                                conflict_found = true;
                                popup( _( "The trait [%1$s] conflicts with hobby [%2$s]'s trait [%3$s]." ), new_trait->name(),
                                       hobby->gender_appropriate_name( u.male ), suspect_trait->name() );
                            }
                        }
                    }
                }
            }
            if( conflict_found ) {
                continue;
            }

            // Toggle hobby
            if( u.hobbies.count( prof ) == 0 ) {
                // Add hobby, and decrement point cost
                u.hobbies.insert( prof );
            } else {
                // Remove hobby and refund point cost
                u.hobbies.erase( prof );
            }

            // Add or remove traits from hobby
            for( const trait_id &trait : prof->get_locked_traits() ) {
                u.toggle_trait( trait );
            }

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
            .description( _( "Search by hobby name." ) )
            .edit( filterstring );
            recalc_profs = true;
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, profs_length - 1 );
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

tab_direction set_skills( avatar &u, pool_type pool )
{
    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    int iContentHeight = 0;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - 6;
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( iContentHeight - 5, TERMX - 35, point( 31, 5 ) );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    auto sorted_skills = Skill::get_skills_sorted_by( []( const Skill & a, const Skill & b ) {
        return localized_compare( a.name(), b.name() );
    } );

    const int num_skills = sorted_skills.size();
    int cur_offset = 0;
    int cur_pos = 0;
    const Skill *currentSkill = sorted_skills[cur_pos];
    int selected = 0;

    input_context ctxt( "NEW_CHAR_SKILLS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "SCROLL_SKILL_INFO_DOWN" );
    ctxt.register_action( "SCROLL_SKILL_INFO_UP" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    std::map<skill_id, int> prof_skills;
    const auto &pskills = u.prof->skills();

    std::copy( pskills.begin(), pskills.end(),
               std::inserter( prof_skills, prof_skills.begin() ) );

    const int remaining_points_length = utf8_width( pools_to_string( u, pool ), true );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_character_tabs( w, _( "SKILLS" ) );

        // Helptext skills tab
        fold_and_print( w, point( 2, TERMY - 5 ), getmaxx( w ) - 4, COL_NOTE_MINOR,
                        _( "Press <color_light_green>%s</color> to view and alter keybindings.\n"
                           "Press <color_light_green>%s</color> / <color_light_green>%s</color> to select skill.\n"
                           "Press <color_light_green>%s</color> to increase skill or "
                           "<color_light_green>%s</color> to decrease skill.\n"
                           "Press <color_light_green>%s</color> to go to the next tab or "
                           "<color_light_green>%s</color> to return to the previous tab." ),
                        ctxt.get_desc( "HELP_KEYBINDINGS" ), ctxt.get_desc( "UP" ), ctxt.get_desc( "DOWN" ),
                        ctxt.get_desc( "RIGHT" ), ctxt.get_desc( "LEFT" ),
                        ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "PREV_TAB" ) );

        draw_points( w, pool, u );
        // Clear the bottom of the screen.
        werase( w_description );
        mvwprintz( w, point( remaining_points_length + 9, 3 ), c_light_gray,
                   std::string( getmaxx( w ) - remaining_points_length - 10, ' ' ) );

        // Write the hint as to upgrade costs
        const int cost = skill_increment_cost( u, currentSkill->ident() );
        const int level = u.get_skill_level( currentSkill->ident() );
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

                recipes[r.skill_used->name()].emplace_back(
                    r.result_name( /*decorated=*/true ),
                    ( skill > 0 ) ? skill : r.difficulty
                );
            }
        }

        std::string rec_disp;

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
                rec_disp = "\n\n" + colorize( rec_temp, c_brown ) + std::move( rec_disp );
            } else {
                rec_disp += "\n\n" + colorize( "[" + elem.first + "]\n" + rec_temp, c_light_gray );
            }
        }

        rec_disp = currentSkill->description() + rec_disp;

        const auto vFolded = foldstring( rec_disp, getmaxx( w_description ) );
        int iLines = vFolded.size();

        if( selected < 0 || iLines < iContentHeight ) {
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
                           ( i == cur_pos ? COL_SELECT : c_light_gray ), thisSkill->name() );
            } else {
                mvwprintz( w, point( 2, y ),
                           ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ),
                           thisSkill->name() );
                wprintz( w, ( i == cur_pos ? hilite( COL_SKILL_USED ) : COL_SKILL_USED ),
                         " ( %d )", u.get_skill_level( thisSkill->ident() ) );
            }

            int skill_level = 0;

            // Grab skills from profession
            for( auto &prof_skill : u.prof->skills() ) {
                if( prof_skill.first == thisSkill->ident() ) {
                    skill_level += prof_skill.second;
                    break;
                }
            }

            // Only show bonus if we are above 0
            if( skill_level > 0 ) {
                wprintz( w, ( i == cur_pos ? h_white : c_white ), " (+%d)", skill_level );
            }

        }

        draw_scrollbar( w, cur_pos, iContentHeight, num_skills, point( 0, 5 ) );

        wnoutrefresh( w );
        wnoutrefresh( w_description );
    } );

    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int scroll_rate = num_skills > 20 ? 5 : 2;
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
        } else if( action == "PAGE_DOWN" ) {
            if( cur_pos == num_skills - 1 ) {
                cur_pos = 0;
            } else if( cur_pos + scroll_rate >= num_skills ) {
                cur_pos = num_skills - 1;
            } else {
                cur_pos += +scroll_rate;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if( action == "PAGE_UP" ) {
            if( cur_pos == 0 ) {
                cur_pos = num_skills - 1;
            } else if( cur_pos <= scroll_rate ) {
                cur_pos = 0;
            } else {
                cur_pos += -scroll_rate;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if( action == "LEFT" ) {
            const skill_id &skill_id = currentSkill->ident();
            const int level = u.get_skill_level( skill_id );
            if( level > 0 ) {
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free, but
                // decreasing it from level 2 forfeits the free extra level (thus changes it to 0)
                u.mod_skill_level( skill_id, level == 2 ? -2 : -1 );
                u.set_knowledge_level( skill_id, u.get_skill_level( skill_id ) );
            }
        } else if( action == "RIGHT" ) {
            const skill_id &skill_id = currentSkill->ident();
            const int level = u.get_skill_level( skill_id );
            if( level < MAX_SKILL ) {
                // For balance reasons, increasing a skill from level 0 gives 1 extra level for free
                u.mod_skill_level( skill_id, level == 0 ? +2 : +1 );
                u.set_knowledge_level( skill_id, u.get_skill_level( skill_id ) );
            }
        } else if( action == "SCROLL_SKILL_INFO_DOWN" ) {
            selected++;
        } else if( action == "SCROLL_SKILL_INFO_UP" ) {
            selected--;
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "NEXT_TAB" ) {
            return tab_direction::FORWARD;
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

static struct {
    bool sort_by_points = true;
    bool male = false;
    bool cities_enabled = false;
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
            return localized_compare( a->gender_appropriate_name( male ),
                                      b->gender_appropriate_name( male ) );
        }
    }
} scenario_sorter;

tab_direction set_scenario( avatar &u, pool_type pool )
{
    int cur_id = 0;
    tab_direction retval = tab_direction::NONE;
    int iContentHeight = 0;
    int iStartPos = 0;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_description;
    catacurses::window w_sorting;
    catacurses::window w_profession;
    catacurses::window w_location;
    catacurses::window w_vehicle;
    catacurses::window w_initial_date;
    catacurses::window w_flags;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        iContentHeight = TERMY - 10;
        w = catacurses::newwin( TERMY, TERMX, point_zero );
        w_description = catacurses::newwin( 4, TERMX - 2, point( 1, TERMY - 5 ) );
        const int second_column_w = TERMX / 2 - 1;
        point origin = point( second_column_w + 1, 5 );
        const int w_sorting_h = 2;
        const int w_profession_h = 4;
        const int w_location_h = 3;
        const int w_vehicle_h = 3;
        const int w_initial_date_h = 6;
        const int w_flags_h = clamp<int>( 0,
                                          iContentHeight -
                                          ( w_sorting_h + w_profession_h + w_location_h + w_vehicle_h + w_initial_date_h ),
                                          iContentHeight );
        w_sorting = catacurses::newwin( w_sorting_h, second_column_w, origin );
        origin += point( 0, w_sorting_h );

        w_profession = catacurses::newwin( w_profession_h, second_column_w, origin );
        origin += point( 0, w_profession_h );

        w_location = catacurses::newwin( w_location_h, second_column_w, origin );
        origin += point( 0, w_location_h );

        w_vehicle = catacurses::newwin( w_vehicle_h, second_column_w, origin );
        origin += point( 0, w_vehicle_h );

        w_initial_date = catacurses::newwin( w_initial_date_h, second_column_w, origin );
        origin += point( 0, w_initial_date_h );

        w_flags = catacurses::newwin( w_flags_h, second_column_w, origin );
        ui.position_from_window( w );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "NEW_CHAR_SCENARIOS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "RANDOMIZE" );

    bool recalc_scens = true;
    int scens_length = 0;
    std::string filterstring;
    std::vector<const scenario *> sorted_scens;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );
        draw_character_tabs( w, _( "SCENARIO" ) );

        // Draw filter indicator
        for( int i = 1; i < getmaxx( w ) - 1; i++ ) {
            mvwputch( w, point( i, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXOX );
        }
        const auto filter_indicator = filterstring.empty() ? _( "no filter" )
                                      : filterstring;
        mvwprintz( w, point( 2, getmaxy( w ) - 1 ), c_light_gray, "<%s>", filter_indicator );

        const bool cur_id_is_valid = cur_id >= 0 && static_cast<size_t>( cur_id ) < sorted_scens.size();

        werase( w_description );
        if( cur_id_is_valid ) {
            int netPointCost = sorted_scens[cur_id]->point_cost() - get_scenario()->point_cost();
            bool can_pick = sorted_scens[cur_id]->can_pick(
                                *get_scenario(),
                                skill_points_left( u, pool ) );
            const std::string clear_line( getmaxx( w_description ), ' ' );

            // Clear the bottom of the screen and header.
            mvwprintz( w, point( 1, 3 ), c_light_gray, clear_line );

            int pointsForScen = sorted_scens[cur_id]->point_cost();
            bool negativeScen = pointsForScen < 0;
            if( negativeScen ) {
                pointsForScen *= -1;
            }

            // Draw header.
            draw_points( w, pool, u, netPointCost );

            const char *scen_msg_temp;
            if( isWide ) {
                if( negativeScen ) {
                    //~ 1s - scenario name, 2d - current character points.
                    scen_msg_temp = n_gettext( "Scenario %1$s earns %2$d point",
                                               "Scenario %1$s earns %2$d points",
                                               pointsForScen );
                } else {
                    //~ 1s - scenario name, 2d - current character points.
                    scen_msg_temp = n_gettext( "Scenario %1$s costs %2$d point",
                                               "Scenario %1$s costs %2$d points",
                                               pointsForScen );
                }
            } else {
                if( negativeScen ) {
                    scen_msg_temp = n_gettext( "Scenario earns %2$d point",
                                               "Scenario earns %2$d points", pointsForScen );
                } else {
                    scen_msg_temp = n_gettext( "Scenario costs %2$d point",
                                               "Scenario costs %2$d points", pointsForScen );
                }
            }

            int pMsg_length = utf8_width( remove_color_tags( pools_to_string( u, pool ) ) );
            mvwprintz( w, point( pMsg_length + 9, 3 ), can_pick ? c_green : c_light_red, scen_msg_temp,
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
            if( get_scenario() != sorted_scens[i] ) {
                if( cur_id_is_valid && sorted_scens[i] == sorted_scens[cur_id] &&
                    sorted_scens[i]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
                    col = h_dark_gray;
                } else if( cur_id_is_valid && sorted_scens[i] != sorted_scens[cur_id] &&
                           sorted_scens[i]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
                    col = c_dark_gray;
                } else {
                    col = ( cur_id_is_valid && sorted_scens[i] == sorted_scens[cur_id] ? COL_SELECT : c_light_gray );
                }
            } else {
                col = ( cur_id_is_valid &&
                        sorted_scens[i] == sorted_scens[cur_id] ? hilite( COL_SKILL_USED ) : COL_SKILL_USED );
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
        werase( w_vehicle );
        werase( w_initial_date );
        werase( w_flags );

        if( cur_id_is_valid ) {
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
            wprintz( w_location, c_light_gray,
                     string_format( _( "%s (%d locations, %d variants)" ),
                                    sorted_scens[cur_id]->start_name(),
                                    sorted_scens[cur_id]->start_location_count(),
                                    sorted_scens[cur_id]->start_location_targets_count() ) );

            mvwprintz( w_vehicle, point_zero, COL_HEADER, _( "Scenario Vehicle:" ) );
            wprintz( w_vehicle, c_light_gray, ( "\n" ) );
            if( sorted_scens[cur_id]->vehicle() ) {
                wprintz( w_vehicle, c_light_gray, "%s", sorted_scens[cur_id]->vehicle()->name );
            }

            mvwprintz( w_initial_date, point_zero, COL_HEADER, _( "Scenario calendar:" ) );
            wprintz( w_initial_date, c_light_gray, ( "\n" ) );
            if( sorted_scens[cur_id]->custom_start_date() ) {
                wprintz( w_initial_date, c_light_gray,
                         sorted_scens[cur_id]->is_random_year() ? _( "Year:   Random" ) : _( "Year:   %s" ),
                         sorted_scens[cur_id]->start_year() );
                wprintz( w_initial_date, c_light_gray, ( "\n" ) );
                wprintz( w_initial_date, c_light_gray, _( "Season: %s" ),
                         calendar::name_season( sorted_scens[cur_id]->start_season() ) );
                wprintz( w_initial_date, c_light_gray, ( "\n" ) );
                wprintz( w_initial_date, c_light_gray,
                         sorted_scens[cur_id]->is_random_day() ? _( "Day:    Random" ) : _( "Day:    %d" ),
                         sorted_scens[cur_id]->day_of_season() );
                wprintz( w_initial_date, c_light_gray, ( "\n" ) );
                wprintz( w_initial_date, c_light_gray,
                         sorted_scens[cur_id]->is_random_hour() ? _( "Hour:   Random" ) : _( "Hour:   %d" ),
                         sorted_scens[cur_id]->start_hour() );
                wprintz( w_initial_date, c_light_gray, ( "\n" ) );
            } else {
                wprintz( w_initial_date, c_light_gray, _( "Default" ) );
                wprintz( w_initial_date, c_light_gray, ( "\n" ) );
            }

            mvwprintz( w_flags, point_zero, COL_HEADER, _( "Scenario Flags:" ) );
            wprintz( w_flags, c_light_gray, ( "\n" ) );

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
            if( sorted_scens[cur_id]->has_flag( "FUNGAL_INFECTION" ) ) {
                wprintz( w_flags, c_light_gray, _( "Fungal infected player" ) );
                wprintz( w_flags, c_light_gray, ( "\n" ) );
            }
            if( sorted_scens[cur_id]->has_flag( "LONE_START" ) ) {
                wprintz( w_flags, c_light_gray, _( "No starting NPC" ) );
                wprintz( w_flags, c_light_gray, ( "\n" ) );
            }
            if( sorted_scens[cur_id]->has_flag( "BORDERED" ) ) {
                wprintz( w_flags, c_light_gray, _( "Starting location is bordered by an immense wall" ) );
                wprintz( w_flags, c_light_gray, ( "\n" ) );
            }
        }

        draw_scrollbar( w, cur_id, iContentHeight, scens_length, point( 0, 5 ) );
        wnoutrefresh( w );
        wnoutrefresh( w_description );
        wnoutrefresh( w_sorting );
        wnoutrefresh( w_profession );
        wnoutrefresh( w_location );
        wnoutrefresh( w_vehicle );
        wnoutrefresh( w_initial_date );
        wnoutrefresh( w_flags );
    } );

    do {
        if( recalc_scens ) {
            sorted_scens.clear();
            auto &wopts = world_generator->active_world->WORLD_OPTIONS;
            for( const auto &scen : scenario::get_all() ) {
                if( scen.scen_is_blacklisted() ) {
                    continue;
                }
                if( !lcmatch( scen.gender_appropriate_name( u.male ), filterstring ) ) {
                    continue;
                }
                sorted_scens.push_back( &scen );
            }
            if( sorted_scens.empty() ) {
                popup( _( "Nothing found." ) ); // another case of black box in tiles
                filterstring.clear();
                continue;
            }
            scens_length = static_cast<int>( sorted_scens.size() );

            // Sort scenarios by points.
            // scenario_display_sort() keeps "Evacuee" at the top.
            scenario_sorter.male = u.male;
            scenario_sorter.cities_enabled = wopts["CITY_SIZE"].getValue() != "0";
            std::stable_sort( sorted_scens.begin(), sorted_scens.end(), scenario_sorter );

            // If city size is 0 but the current scenario requires cities reset the scenario
            if( !scenario_sorter.cities_enabled && get_scenario()->has_flag( "CITY_START" ) ) {
                reset_scenario( u, sorted_scens[0] );
            }

            // Select the current scenario, if possible.
            for( int i = 0; i < scens_length; ++i ) {
                if( sorted_scens[i]->ident() == get_scenario()->ident() ) {
                    cur_id = i;
                    break;
                }
            }
            if( cur_id > scens_length - 1 ) {
                cur_id = 0;
            }

            recalc_scens = false;
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int scroll_rate = scens_length > 20 ? 5 : 2;
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
        } else if( action == "PAGE_DOWN" ) {
            if( cur_id == scens_length - 1 ) {
                cur_id = 0;
            } else if( cur_id + scroll_rate >= scens_length ) {
                cur_id = scens_length - 1;
            } else {
                cur_id += +scroll_rate;
            }
        } else if( action == "PAGE_UP" ) {
            if( cur_id == 0 ) {
                cur_id = scens_length - 1;
            } else if( cur_id <= scroll_rate ) {
                cur_id = 0;
            } else {
                cur_id += -scroll_rate;
            }
        } else if( action == "CONFIRM" ) {
            if( sorted_scens[cur_id]->has_flag( "CITY_START" ) && !scenario_sorter.cities_enabled ) {
                continue;
            }
            reset_scenario( u, sorted_scens[cur_id] );
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
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            retval = tab_direction::QUIT;
        } else if( action == "RANDOMIZE" ) {
            cur_id = rng( 0, scens_length - 1 );
        }
    } while( retval == tab_direction::NONE );

    return retval;
}

namespace char_creation
{
enum description_selector {
    NAME,
    GENDER,
    HEIGHT,
    AGE,
    BLOOD,
    LOCATION
};

static void draw_gender( const catacurses::window &w_gender, const avatar &you,
                         const bool highlight )
{
    unsigned male_pos = 1 + utf8_width( _( "Gender:" ) );
    unsigned female_pos = 2 + male_pos + utf8_width( _( "Male" ) );

    werase( w_gender );
    mvwprintz( w_gender, point_zero, highlight ? COL_SELECT : c_light_gray, _( "Gender:" ) );
    mvwprintz( w_gender, point( male_pos, 0 ), ( you.male ? c_light_cyan : c_light_gray ),
               _( "Male" ) );
    mvwprintz( w_gender, point( female_pos, 0 ), ( you.male ? c_light_gray : c_pink ),
               _( "Female" ) );
    wnoutrefresh( w_gender );
}

static void draw_height( const catacurses::window &w_height, const avatar &you,
                         const bool highlight )
{
    werase( w_height );
    mvwprintz( w_height, point_zero, highlight ? COL_SELECT : c_light_gray, _( "Height:" ) );
    unsigned height_pos = 1 + utf8_width( _( "Height:" ) );
    mvwprintz( w_height, point( height_pos, 0 ), c_white, string_format( "%d cm",
               you.base_height() ) );
    wnoutrefresh( w_height );
}

static void draw_age( const catacurses::window &w_age, const avatar &you, const bool highlight )
{
    werase( w_age );
    mvwprintz( w_age, point_zero, highlight ? COL_SELECT : c_light_gray, _( "Age:" ) );
    unsigned age_pos = 1 + utf8_width( _( "Age:" ) );
    mvwprintz( w_age, point( age_pos, 0 ), c_white, string_format( "%d", you.base_age() ) );
    wnoutrefresh( w_age );
}

static void draw_blood( const catacurses::window &w_blood, const avatar &you, const bool highlight )
{
    werase( w_blood );
    mvwprintz( w_blood, point_zero, highlight ? COL_SELECT : c_light_gray, _( "Blood type:" ) );
    unsigned blood_pos = 1 + utf8_width( _( "Blood type:" ) );
    mvwprintz( w_blood, point( blood_pos, 0 ), c_white,
               io::enum_to_string( you.my_blood_type ) + ( you.blood_rh_factor ? "+" : "-" ) );
    wnoutrefresh( w_blood );
}

static void draw_location( const catacurses::window &w_location, const avatar &you,
                           const bool highlight )
{
    const std::string random_start_location_text = string_format( n_gettext(
                "<color_red>* Random location *</color> (<color_white>%d</color> variant)",
                "<color_red>* Random location *</color> (<color_white>%d</color> variants)",
                get_scenario()->start_location_targets_count() ), get_scenario()->start_location_targets_count() );

    werase( w_location );
    mvwprintz( w_location, point_zero, highlight ? COL_SELECT : c_light_gray,
               _( "Starting location:" ) );
    // ::find will return empty location if id was not found. Debug msg will be printed too.
    mvwprintz( w_location, point( utf8_width( _( "Starting location:" ) ) + 1, 0 ),
               you.random_start_location ? c_red : c_white,
               you.random_start_location ? remove_color_tags( random_start_location_text ) :
               string_format( n_gettext( "%s (%d variant)", "%s (%d variants)",
                                         you.start_location.obj().targets_count() ),
                              you.start_location.obj().name(), you.start_location.obj().targets_count() ) );
    wnoutrefresh( w_location );
}

} // namespace char_creation

tab_direction set_description( avatar &you, const bool allow_reroll,
                               pool_type pool )
{
    static constexpr int RANDOM_START_LOC_ENTRY = INT_MIN;

    ui_adaptor ui;
    catacurses::window w;
    catacurses::window w_name;
    catacurses::window w_gender;
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
    const auto init_windows = [&]( ui_adaptor & ui ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );
        const int beginx2 = 46;
        const int ncol2 = 40;
        const int beginx3 = TERMX <= 88 ? TERMX - TERMX / 4 : 86;
        const int ncol3 = TERMX - beginx3 - 2;
        const int beginx4 = TERMX <= 130 ? TERMX - TERMX / 5 : 128;
        const int ncol4 = TERMX - beginx4 - 2;
        const int ncol_small = ( TERMX / 2 ) - 2;
        const int begin_sncol = ( TERMX / 2 );
        if( isWide ) {
            w = catacurses::newwin( TERMY, TERMX, point_zero );
            w_name = catacurses::newwin( 2, ncol2 + 2, point( 2, 5 ) );
            w_gender = catacurses::newwin( 1, ncol2 + 2, point( 2, 7 ) );
            w_location = catacurses::newwin( 1, ncol3, point( beginx3, 5 ) );
            w_vehicle = catacurses::newwin( 1, ncol3, point( beginx3, 6 ) );
            w_addictions = catacurses::newwin( 1, ncol3, point( beginx3, 7 ) );
            w_stats = catacurses::newwin( 6, 20, point( 2, 9 ) );
            w_traits = catacurses::newwin( TERMY - 10, ncol2, point( beginx2, 9 ) );
            w_bionics = catacurses::newwin( TERMY - 10, ncol3, point( beginx3, 9 ) );
            w_proficiencies = catacurses::newwin( TERMY - 20, 19, point( 2, 15 ) );
            w_hobbies = catacurses::newwin( TERMY - 10, ncol4, point( beginx4, 9 ) );
            w_scenario = catacurses::newwin( 1, ncol2, point( beginx2, 3 ) );
            w_profession = catacurses::newwin( 1, ncol3, point( beginx3, 3 ) );
            w_skills = catacurses::newwin( TERMY - 10, 23, point( 22, 9 ) );
            w_guide = catacurses::newwin( 9, TERMX - 3, point( 2, TERMY - 10 ) );
            w_height = catacurses::newwin( 1, ncol2, point( beginx2, 5 ) );
            w_age = catacurses::newwin( 1, ncol2, point( beginx2, 6 ) );
            w_blood = catacurses::newwin( 1, ncol2, point( beginx2, 7 ) );
            ui.position_from_window( w );
        } else {
            w = catacurses::newwin( TERMY, TERMX, point_zero );
            w_name = catacurses::newwin( 1, ncol_small, point( 2, 5 ) );
            w_gender = catacurses::newwin( 1, ncol_small, point( 2, 6 ) );
            w_height = catacurses::newwin( 1, ncol_small, point( 2, 7 ) );
            w_age = catacurses::newwin( 1, ncol_small, point( begin_sncol, 5 ) );
            w_blood = catacurses::newwin( 1, ncol_small, point( begin_sncol, 6 ) );
            w_location = catacurses::newwin( 1, ncol_small, point( begin_sncol, 7 ) );
            w_stats = catacurses::newwin( 6, ncol_small, point( 2, 9 ) );
            w_scenario = catacurses::newwin( 1, ncol_small, point( begin_sncol, 9 ) );
            w_profession = catacurses::newwin( 1, ncol_small, point( begin_sncol, 10 ) );
            w_vehicle = catacurses::newwin( 2, ncol_small, point( begin_sncol, 12 ) );
            w_addictions = catacurses::newwin( 2, ncol_small, point( begin_sncol, 14 ) );
            w_guide = catacurses::newwin( 2, TERMX - 3, point( 2, TERMY - 3 ) );
            ui.position_from_window( w );
        }
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    const unsigned namebar_pos = 1 + utf8_width( _( "Name:" ) );

    input_context ctxt( "NEW_CHAR_DESCRIPTION" );
    ctxt.register_cardinal();
    ctxt.register_action( "SAVE_TEMPLATE" );
    ctxt.register_action( "RANDOMIZE_CHAR_NAME" );
    ctxt.register_action( "RANDOMIZE_CHAR_DESCRIPTION" );
    if( !MAP_SHARING::isSharing() && allow_reroll ) {
        ctxt.register_action( "REROLL_CHARACTER" );
        ctxt.register_action( "REROLL_CHARACTER_WITH_SCENARIO" );
    }
    ctxt.register_action( "CHANGE_GENDER" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CHOOSE_LOCATION" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );

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
    for( const auto &loc : start_locations::get_all() ) {
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
    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_character_tabs( w, _( "DESCRIPTION" ) );

        draw_points( w, pool, you );

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
        wnoutrefresh( w );

        wclear( w_stats );
        wclear( w_traits );
        wclear( w_skills );
        wclear( w_guide );

        std::vector<std::string> vStatNames;
        mvwprintz( w_stats, point_zero, COL_HEADER, _( "Stats:" ) );
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
            mvwprintz( w_traits, point_zero, COL_HEADER, _( "Traits: " ) );
            std::vector<trait_id> current_traits = pool == pool_type::TRANSFER ? you.get_mutations() :
                                                   you.get_base_traits();
            std::sort( current_traits.begin(), current_traits.end(), trait_display_sort );
            if( current_traits.empty() ) {
                wprintz( w_traits, c_light_red, _( "None!" ) );
            } else {
                for( size_t i = 0; i < current_traits.size(); i++ ) {
                    const auto current_trait = current_traits[i];
                    trim_and_print( w_traits, point( 0, i + 1 ), getmaxx( w_traits ) - 1,
                                    current_trait->get_display_color(), current_trait->name() );
                }
            }
        }
        wnoutrefresh( w_traits );

        if( isWide ) {
            mvwprintz( w_skills, point_zero, COL_HEADER, _( "Skills:" ) );

            auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a, const Skill & b ) {
                const int level_a = you.get_skill_level_object( a.ident() ).exercised_level();
                const int level_b = you.get_skill_level_object( b.ident() ).exercised_level();
                return localized_compare( std::make_pair( -level_a, a.name() ),
                                          std::make_pair( -level_b, b.name() ) );
            } );

            int line = 1;
            bool has_skills = false;
            profession::StartingSkillList list_skills = you.prof->skills();

            for( auto &elem : skillslist ) {
                int level = you.get_skill_level( elem->ident() );

                // Handle skills from professions
                if( pool != pool_type::TRANSFER ) {
                    profession::StartingSkillList::iterator i = list_skills.begin();
                    while( i != list_skills.end() ) {
                        if( i->first == ( elem )->ident() ) {
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
                        if( i->first == ( elem )->ident() ) {
                            int skill_exp_bonus = calculate_cumulative_experience( i->second );

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
        }
        wnoutrefresh( w_skills );

        if( isWide ) {
            werase( w_bionics );
            // Profession bionics description tab, active bionics shown first
            auto prof_CBMs = you.prof->CBMs();
            std::sort( begin( prof_CBMs ), end( prof_CBMs ), []( const bionic_id & a, const bionic_id & b ) {
                return a->activated && !b->activated;
            } );
            mvwprintz( w_bionics, point_zero, COL_HEADER, _( "Bionics:" ) );
            if( prof_CBMs.empty() ) {
                mvwprintz( w_bionics, point( utf8_width( _( "Bionics:" ) ) + 1, 0 ), c_light_red, _( "None!" ) );
            } else {
                for( const auto &b : prof_CBMs ) {
                    const auto &cbm = b.obj();

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
        werase( w_proficiencies );
        if( isWide ) {
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

            mvwprintz( w_proficiencies, point_zero, COL_HEADER, _( "Proficiencies:" ) );
            if( prof_proficiencies.empty() ) {
                mvwprintz( w_proficiencies, point_south, c_light_red, _( "None!" ) );
            } else {
                for( const proficiency_id &prof : prof_proficiencies ) {
                    wprintz( w_proficiencies, c_light_gray, "\n" + trim_by_length( prof->name(), 18 ) );
                }
            }
        }
        wnoutrefresh( w_proficiencies );

        // Helptext description window
        if( isWide ) {
            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 9 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                            ctxt.get_desc( "HELP_KEYBINDINGS" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 8 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to save character template." ),
                            ctxt.get_desc( "SAVE_TEMPLATE" ) );

            if( !MAP_SHARING::isSharing() && allow_reroll ) { // no random names when sharing maps
                fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 7 ), ( TERMX ), c_light_gray,
                                _( "Press <color_light_green>%s</color> to pick a random name, "
                                   "<color_light_green>%s</color> to randomize all description values, "
                                   "<color_light_green>%s</color> to randomize all but scenario or "
                                   "<color_light_green>%s</color> to randomize everything." ),
                                ctxt.get_desc( "RANDOMIZE_CHAR_NAME" ),
                                ctxt.get_desc( "RANDOMIZE_CHAR_DESCRIPTION" ),
                                ctxt.get_desc( "REROLL_CHARACTER" ),
                                ctxt.get_desc( "REROLL_CHARACTER_WITH_SCENARIO" ) );
            } else {
                fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 7 ), ( TERMX ), c_light_gray,
                                _( "Press <color_light_green>%s</color> to pick a random name, "
                                   "<color_light_green>%s</color> to randomize all description values." ),
                                ctxt.get_desc( "RANDOMIZE_CHAR_NAME" ),
                                ctxt.get_desc( "RANDOMIZE_CHAR_DESCRIPTION" ) );
            }

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 6 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to switch gender." ),
                            ctxt.get_desc( "CHANGE_GENDER" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 5 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to select a specific starting location." ),
                            ctxt.get_desc( "CHOOSE_LOCATION" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 4 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> or <color_light_green>%s</color> "
                               "to cycle through editable values." ),
                            ctxt.get_desc( "UP" ),
                            ctxt.get_desc( "DOWN" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 3 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> and <color_light_green>%s</color> to change gender, height, age, and blood type." ),
                            ctxt.get_desc( "LEFT" ),
                            ctxt.get_desc( "RIGHT" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 2 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to edit value via popup input." ),
                            ctxt.get_desc( "CONFIRM" ) );

            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 1 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to finish character creation "
                               "or <color_light_green>%s</color> to return to the previous TAB." ),
                            ctxt.get_desc( "NEXT_TAB" ),
                            ctxt.get_desc( "PREV_TAB" ) );
        } else {
            fold_and_print( w_guide, point( 0, getmaxy( w_guide ) - 1 ), ( TERMX ), c_light_gray,
                            _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                            ctxt.get_desc( "HELP_KEYBINDINGS" ) );

        }
        wnoutrefresh( w_guide );

        wclear( w_name );
        mvwprintz( w_name, point_zero,
                   current_selector == char_creation::NAME ? COL_SELECT : c_light_gray, _( "Name:" ) );
        if( no_name_entered ) {
            mvwprintz( w_name, point( namebar_pos, 0 ), COL_SELECT, _( "--- NO NAME ENTERED ---" ) );
        } else if( you.name.empty() ) {
            mvwprintz( w_name, point( namebar_pos, 0 ), c_light_gray, _( "--- RANDOM NAME ---" ) );
        } else {
            mvwprintz( w_name, point( namebar_pos, 0 ), c_white, you.name );
        }

        wnoutrefresh( w_name );

        char_creation::draw_gender( w_gender, you, current_selector == char_creation::GENDER );
        char_creation::draw_age( w_age, you, current_selector == char_creation::AGE );
        char_creation::draw_height( w_height, you, current_selector == char_creation::HEIGHT );
        char_creation::draw_blood( w_blood, you, current_selector == char_creation::BLOOD );
        char_creation::draw_location( w_location, you, current_selector == char_creation::LOCATION );

        werase( w_vehicle );
        // Player vehicle description tab
        const vproto_id scen_veh = get_scenario()->vehicle();
        const vproto_id prof_veh = you.prof->vehicle();
        if( isWide ) {
            if( scen_veh ) {
                mvwprintz( w_vehicle, point_zero, c_light_gray, _( "Starting vehicle (scenario): " ) );
                wprintz( w_vehicle, c_white, "%s", scen_veh->name );
            } else if( prof_veh ) {
                mvwprintz( w_vehicle, point_zero, c_light_gray, _( "Starting vehicle (profession): " ) );
                wprintz( w_vehicle, c_white, "%s", prof_veh->name );
            } else {
                mvwprintz( w_vehicle, point_zero, c_light_gray, _( "Starting vehicle: " ) );
                wprintz( w_vehicle, c_light_red, _( "None!" ) );
            }
        } else {
            if( scen_veh ) {
                mvwprintz( w_vehicle, point_zero, COL_HEADER, _( "Starting vehicle (scenario): " ) );
                wprintz( w_vehicle, c_light_gray, "\n%s", scen_veh->name );
            } else if( prof_veh ) {
                mvwprintz( w_vehicle, point_zero, COL_HEADER, _( "Starting vehicle (profession): " ) );
                wprintz( w_vehicle, c_light_gray, "\n%s", prof_veh->name );
            } else {
                mvwprintz( w_vehicle, point_zero, COL_HEADER, _( "Starting vehicle: " ) );
                wprintz( w_vehicle, c_light_red, _( "None!" ) );
            }
        }
        wnoutrefresh( w_vehicle );

        werase( w_addictions );
        // Profession addictions description tab
        const auto prof_addictions = you.prof->addictions();
        if( isWide ) {
            if( prof_addictions.empty() ) {
                mvwprintz( w_addictions, point_zero, c_light_gray, _( "Starting addictions: " ) );
                wprintz( w_addictions, c_light_red, _( "None!" ) );
            } else {
                mvwprintz( w_addictions, point_zero, c_light_gray, _( "Starting addictions: " ) );
                for( const addiction &a : prof_addictions ) {
                    const char *format = "%1$s (%2$d) ";
                    wprintz( w_addictions, c_white, string_format( format, addiction_name( a ), a.intensity ) );
                }
            }
        } else {
            if( prof_addictions.empty() ) {
                mvwprintz( w_addictions, point_zero, COL_HEADER, _( "Starting addictions: " ) );
                wprintz( w_addictions, c_light_red, _( "None!" ) );
            } else {
                mvwprintz( w_addictions, point_zero, COL_HEADER, _( "Starting addictions: " ) );
                for( const addiction &a : prof_addictions ) {
                    const char *format = "%1$s (%2$d) ";
                    wprintz( w_addictions, c_light_gray, ( "\n" ) );
                    wprintz( w_addictions, c_light_gray, string_format( format, addiction_name( a ), a.intensity ) );
                }
            }
        }
        wnoutrefresh( w_addictions );

        werase( w_scenario );
        mvwprintz( w_scenario, point_zero, COL_HEADER, _( "Scenario: " ) );
        wprintz( w_scenario, c_light_gray, get_scenario()->gender_appropriate_name( you.male ) );
        wnoutrefresh( w_scenario );

        werase( w_profession );
        mvwprintz( w_profession, point_zero, COL_HEADER, _( "Profession: " ) );
        wprintz( w_profession, c_light_gray, you.prof->gender_appropriate_name( you.male ) );
        wnoutrefresh( w_profession );

        werase( w_hobbies );
        mvwprintz( w_hobbies, point_zero, COL_HEADER, _( "Hobbies: " ) );
        if( you.hobbies.empty() ) {
            mvwprintz( w_hobbies, point_south, c_light_red, _( "None!" ) );
        } else {
            for( const profession *prof : you.hobbies ) {
                wprintz( w_hobbies, c_light_gray, "\n%s", prof->gender_appropriate_name( you.male ) );
            }
        }
        wnoutrefresh( w_hobbies );
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
                    return tab_direction::FORWARD;
                }
            }
            if( query_yn( _( "Are you SURE you're finished?" ) ) ) {
                return tab_direction::FORWARD;
            }
            continue;
        } else if( action == "PREV_TAB" ) {
            return tab_direction::BACKWARD;
        } else if( action == "DOWN" ) {
            switch( current_selector ) {
                case char_creation::NAME:
                    current_selector = char_creation::GENDER;
                    break;
                case char_creation::GENDER:
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
                default:
                    break;
            }
        } else if( action == "LEFT" ) {
            switch( current_selector ) {
                case char_creation::HEIGHT:
                    if( you.base_height() > min_allowed_height ) {
                        you.mod_base_height( -1 );
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
                default:
                    break;
            }
        } else if( action == "REROLL_CHARACTER" && allow_reroll ) {
            you.randomize( false );
            // Return tab_direction::NONE so we re-enter this tab again, but it forces a complete redrawing of it.
            return tab_direction::NONE;
        } else if( action == "REROLL_CHARACTER_WITH_SCENARIO" && allow_reroll ) {
            you.randomize( true );
            // Return tab_direction::NONE so we re-enter this tab again, but it forces a complete redrawing of it.
            return tab_direction::NONE;
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
            you.male = one_in( 2 );
            if( !MAP_SHARING::isSharing() ) { // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                you.pick_name();
                no_name_entered = you.name.empty();
            }
            you.set_base_age( rng( 16, 55 ) );
            you.randomize_height();
            you.randomize_blood();
        } else if( action == "CHANGE_GENDER" ) {
            you.male = !you.male;
        } else if( action == "CHOOSE_LOCATION" ) {
            select_location.query();
            if( select_location.ret == RANDOM_START_LOC_ENTRY ) {
                you.random_start_location = true;
            } else if( select_location.ret >= 0 ) {
                for( const auto &loc : start_locations::get_all() ) {
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
                    popup.title( _( "Enter age in years.  Minimum 16, maximum 55" ) )
                    .text( string_format( "%d", you.base_age() ) )
                    .only_digits( true );
                    const int result = popup.query_int();
                    if( result != 0 ) {
                        you.set_base_age( clamp( result, 16, 55 ) );
                    }
                    break;
                }
                case char_creation::HEIGHT: {
                    popup.title( string_format( _( "Enter height in centimeters.  Minimum %d, maximum %d" ),
                                                min_allowed_height,
                                                max_allowed_height ) )
                    .text( string_format( "%d", you.base_height() ) )
                    .only_digits( true );
                    const int result = popup.query_int();
                    if( result != 0 ) {
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
                case char_creation::LOCATION: {
                    select_location.query();
                    if( select_location.ret == RANDOM_START_LOC_ENTRY ) {
                        you.random_start_location = true;
                    } else if( select_location.ret >= 0 ) {
                        for( const auto &loc : start_locations::get_all() ) {
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
        } else if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
            return tab_direction::QUIT;
        }
    } while( true );
}

std::vector<trait_id> Character::get_base_traits() const
{
    return std::vector<trait_id>( my_traits.begin(), my_traits.end() );
}

std::vector<trait_id> Character::get_mutations( bool include_hidden ) const
{
    std::vector<trait_id> result;
    result.reserve( my_mutations.size() + enchantment_cache->get_mutations().size() );
    for( const std::pair<const trait_id, trait_data> &t : my_mutations ) {
        if( include_hidden || t.first.obj().player_display ) {
            result.push_back( t.first );
        }
    }
    for( const trait_id &ench_trait : enchantment_cache->get_mutations() ) {
        if( include_hidden || ench_trait->player_display ) {
            bool found = false;
            for( const trait_id &exist : result ) {
                if( exist == ench_trait ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                result.push_back( ench_trait );
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
        const trait_id trait = my_mutations.begin()->first;
        my_mutations.erase( my_mutations.begin() );
        mutation_loss_effect( trait );
    }
    cached_mutations.clear();
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
    for( const trait_id &tr : get_avatar().prof->get_locked_traits() ) {
        if( !has_trait( tr ) ) {
            toggle_trait( tr );
        }
    }
    for( const trait_id &tr : get_scenario()->get_locked_traits() ) {
        if( !has_trait( tr ) ) {
            toggle_trait( tr );
        }
    }
}

trait_id Character::random_good_trait()
{
    return get_random_trait( []( const mutation_branch & mb ) {
        return mb.points > 0;
    } );
}

trait_id Character::random_bad_trait()
{
    return get_random_trait( []( const mutation_branch & mb ) {
        return mb.points < 0;
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

void Character::randomize_cosmetic_trait( std::string mutation_type )
{
    trait_id trait = get_random_trait( [mutation_type]( const mutation_branch & mb ) {
        return mb.points == 0 && mb.types.count( mutation_type );
    } );

    if( !has_conflicting_trait( trait ) ) {
        toggle_trait( trait );
    }
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
        spop.callbacks[character] = []() {
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

void avatar::character_to_template( const std::string &name )
{
    save_template( name, pool_type::TRANSFER );
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
        jsout.end_object();

        serialize( jsout );

        jsout.end_array();
    }, _( "player template" ) );
}

bool avatar::load_template( const std::string &template_name, pool_type &pool )
{
    return read_from_file_json( PATH_INFO::templatedir() + template_name +
    ".template", [&]( JsonIn & jsin ) {

        if( jsin.test_array() ) {
            // not a legacy template
            jsin.start_array();

            if( jsin.end_array() ) {
                return;
            }

            JsonObject jobj = jsin.get_object();

            jobj.get_int( "stat_points", 0 );
            jobj.get_int( "trait_points", 0 );
            jobj.get_int( "skill_points", 0 );

            pool = static_cast<pool_type>( jobj.get_int( "limit" ) );

            random_start_location = jobj.get_bool( "random_start_location", true );
            const std::string jobj_start_location = jobj.get_string( "start_location", "" );

            // get_scenario()->allowed_start( loc.ident() ) is checked once scenario loads in avatar::load()
            for( const auto &loc : start_locations::get_all() ) {
                if( loc.id.str() == jobj_start_location ) {
                    random_start_location = false;
                    this->start_location = loc.id;
                    break;
                }
            }

            if( jsin.end_array() ) {
                return;
            }
        }

        deserialize( jsin.get_object() );

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
    for( auto &t : u.get_mutations() ) {
        if( t.obj().hp_modifier.has_value() ) {
            u.toggle_trait( t );
        }
    }

    u.hobbies.clear();
    u.clear_mutations();
    u.recalc_hp();
    u.empty_skills();
    u.add_traits();
}
