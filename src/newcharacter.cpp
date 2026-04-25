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
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_creator_ui.h"
#include "character_martial_arts.h"
#include "city.h"
#include "color.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enum_traits.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "item.h"
#include "json.h"
#include "loading_ui.h"
#include "localized_comparator.h"
#include "magic.h"
#include "magic_enchantment.h"
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
#include "overmapbuffer.h"
#include "path_info.h"
#include "pimpl.h"
#include "player_difficulty.h"
#include "profession.h"
#include "profession_group.h"
#include "proficiency.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "regional_settings.h"
#include "ret_val.h"
#include "rng.h"
#include "scenario.h"
#include "skill.h"
#include "start_location.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "trait_group.h"
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

static const flag_id json_flag_WET( "WET" );
static const flag_id json_flag_auto_wield( "auto_wield" );
static const flag_id json_flag_no_auto_equip( "no_auto_equip" );

static const json_character_flag json_flag_BIONIC_TOGGLED( "BIONIC_TOGGLED" );

static const matype_id style_none( "style_none" );

static const profession_group_id
profession_group_adult_basic_background( "adult_basic_background" );

static const trait_group::Trait_group_tag
Trait_group_NPC_starting_traits( "NPC_starting_traits" );

static const trait_id trait_SMELLY( "SMELLY" );
static const trait_id trait_WEAKSCENT( "WEAKSCENT" );
static const trait_id trait_XS( "XS" );
static const trait_id trait_XXXL( "XXXL" );

static character_creator_uistate cc_uistate;

// Whether or not use Outfit (M) at character creation
static bool outfit = true;

static constexpr int RANDOM_START_LOC_ENTRY = INT_MIN;

// Colors used in this file: (Most else defaults to c_light_gray)
#define COL_STAT_BONUS      c_light_green // Bonus
#define COL_STAT_NEUTRAL    c_white   // Neutral Property
#define COL_STAT_PENALTY    c_light_red   // Penalty
#define COL_HEADER          c_white   // Captions, like "Profession items"
#define COL_NOTE_MINOR      c_light_gray  // Just regular note
#define COL_SELECTED        c_green  // uilist selected
#define COL_NOT_SELECTED    c_light_gray  // uilist not selected

static void draw_colored_text_wrap( const std::string &original_text, nc_color color )
{
    cataimgui::draw_colored_text( original_text, color, ImGui::GetContentRegionAvail().x );
}

static bool cities_enabled()
{
    options_manager::options_container &wopts = world_generator->active_world->WORLD_OPTIONS;
    return wopts["CITY_SIZE"].getValue() != "0";
}

static void draw_spacer()
{
    ImGui::NewLine();
}

static void set_detail_scroll()
{
    if( cc_uistate.scrolled_down ) {
        cc_uistate.scrolled_down = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() + ImGui::GetContentRegionAvail().y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    } else if( cc_uistate.scrolled_up ) {
        cc_uistate.scrolled_up = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() - ImGui::GetContentRegionAvail().y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    }
}

static void setup_list_detail_ui( const std::string &header = std::string(),
                                  float uilist_width = 0.33f )
{
    ImGui::TableSetupScrollFreeze( 0, 1 ); // Make top row always visible
    // reserve space for uilist
    ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthStretch, uilist_width );
    ImGui::TableSetupColumn( header.empty() ? _( "No entries found!" ) : header.c_str(),
                             ImGuiTableColumnFlags_WidthStretch, 1.0f - uilist_width );
    ImGui::TableHeadersRow();
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();

    set_detail_scroll();
}

static std::string get_character_stat_name( int selected_stat_index )
{
    return uppercase_first_letter( io::enum_to_full_string(
                                       static_cast<character_stat>( selected_stat_index ) ) );
}

static std::string get_skill_entry_text( const skill_id selected_skill, const avatar &u )
{
    int prof_skill_level = 0;
    for( auto &prof_skill : u.prof->skills() ) {
        if( prof_skill.first == selected_skill ) {
            prof_skill_level += prof_skill.second;
            break;
        }
    }
    for( const profession *hobby : u.hobbies ) {
        for( auto &hobby_skill : hobby->skills() ) {
            if( hobby_skill.first == selected_skill ) {
                prof_skill_level = std::max( prof_skill_level, hobby_skill.second );
                break;
            }
        }
    }

    const int my_skill = u.get_skill_level( selected_skill );
    auto green_if_positive = [&my_skill, &prof_skill_level]( const std::string & text ) {
        if( my_skill + prof_skill_level > 0 ) {
            return colorize( text, c_green );
        }
        return text;
    };
    std::string skill_entry_text = prof_skill_level == 0 ?
                                   string_format( "%s (%d)", selected_skill->name(), my_skill ) :
                                   string_format( "%s (%d + %d)", selected_skill->name(), my_skill, prof_skill_level );

    return green_if_positive( skill_entry_text );
}

static uilist_entry get_uilist_entry( const std::string &name )
{
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    uilist_entry cc_entry( screen_reader_mode ? std::string() : name );
    cc_entry.hotkey = input_event();

    return cc_entry;
}

static void append_screen_reader_active( std::string &entry_text )
{
    if( get_option<bool>( "SCREEN_READER_MODE" ) ) {
        entry_text = entry_text.append( string_format( " - %s", _( "active" ) ) );
    }
}

static void set_stat_base( avatar &u, character_stat stat, int amt )
{
    // minimum stat specific to character creator UI
    if( amt < CHARACTER_STAT_MIN ) {
        return;
    }
    switch( stat ) {
        case character_stat::STRENGTH:
            u.set_str_base( amt );
            break;
        case character_stat::DEXTERITY:
            u.set_dex_base( amt );
            break;
        case character_stat::INTELLIGENCE:
            u.set_int_base( amt );
            break;
        case character_stat::PERCEPTION:
            u.set_per_base( amt );
            break;
        default:
            break;
    }
}

static int stat_point_pool()
{
    return 4 * 8 + get_option<int>( "INITIAL_STAT_POINTS" );
}
static int stat_points_used( const Character &u )
{
    int used = 0;
    for( int stat : {
             u.get_str_base(), u.get_dex_base(), u.get_int_base(), u.get_per_base()
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
    if( one_in( 10 ) ) {
        outfit = !gender_selection;
    } else {
        outfit = gender_selection;
    }
    if( !MAP_SHARING::isSharing() ) {
        play_now ? pick_name() : pick_name( true );
    } else {
        name = MAP_SHARING::getUsername();
    }
    randomize_height();
    randomize_blood();
    randomize_heartrate();
    bool cities_enabled = overmap_buffer.get_settings(
                              this->pos_abs_omt() ).get_settings_city().city_size != 0;
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
    prof = scenario_from->weighted_random_profession( is_npc() );
    play_name_suffix = prof->gender_appropriate_name( male );
    zero_all_skills();

    init_age = rng( this->prof->age_lower, this->prof->age_upper );
    starting_city = std::nullopt;
    world_origin = std::nullopt;
    random_start_location = true;

    set_str_base( rng( 6, HIGH_STAT - 2 ) );
    set_dex_base( rng( 6, HIGH_STAT - 2 ) );
    set_int_base( rng( 6, HIGH_STAT - 2 ) );
    set_per_base( rng( 6, HIGH_STAT - 2 ) );

    set_body();
    randomize_hobbies();
    if( is_npc() ) {
        const trait_id background = prof->pick_background();
        if( !background.is_empty() ) {
            set_mutation( background );
        }
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
                    if( get_str_base() > 5 ) {
                        set_str_base( get_str_base() - 1 );
                    }
                    break;
                case 2:
                    if( get_dex_base() > 5 ) {
                        set_dex_base( get_dex_base() - 1 );
                    }
                    break;
                case 3:
                    if( get_int_base() > 5 ) {
                        set_int_base( get_int_base() - 1 );
                    }
                    break;
                case 4:
                    if( get_per_base() > 5 ) {
                        set_per_base( get_per_base() - 1 );
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
                            set_str_base( get_str_base() + 1 );
                            break;
                        case 2:
                            set_dex_base( get_dex_base() + 1 );
                            break;
                        case 3:
                            set_int_base( get_int_base() + 1 );
                            break;
                        case 4:
                            set_per_base( get_per_base() + 1 );
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
        for( const trait_and_var &cur : trait_group::traits_from( Trait_group_NPC_starting_traits ) ) {
            if( !has_conflicting_trait( cur.trait ) ) {
                set_mutation( cur.trait, cur.trait->variant( cur.variant ) );
            }
        }
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
            if( it.has_flag( json_flag_WET ) ) {
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
    std::vector<profession_id> choices = get_scenario()->permitted_hobbies( is_npc() );
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
    if( choices.empty() ) {
        return;
    }

    const profession_id hobby = random_entry_removed( choices );
    hobbies.insert( &*hobby );

    // Add or remove traits from hobby
    for( const trait_and_var &cur : hobby->get_locked_traits() ) {
        toggle_trait( cur.trait );
    }
}

bool avatar::create( character_type type, const std::string &tempname )
{
    loading_ui::done();
    set_wielded_item( item() );

    prof = profession::generic();
    set_scenario( scenario::generic() );

    const bool interactive = type != character_type::NOW &&
                             type != character_type::FULL_RANDOM;
    const bool skip_to_description = type == character_type::RANDOM ||
                                     type == character_type::TEMPLATE;

    pool_type pool = pool_type::FREEFORM;

    switch( type ) {
        case character_type::CUSTOM:
            if( !get_option<std::string>( "DEF_CHAR_NAME" ).empty() ) {
                name = get_option<std::string>( "DEF_CHAR_NAME" );
            }
            randomize_cosmetics();
            break;
        case character_type::RANDOM:
            //random scenario, default name if exist
            randomize( true );
            //tabs.position.last();
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
            //tabs.position.last();
            break;
    }
    // Don't apply the default backgrounds on a template or scenario with SKIP_DEFAULT_BACKGROUND
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

    if( pool == pool_type::TRANSFER ) {
        return true;
    }

    if( !interactive ) {
        // no window is created because "Play now" does not require any configuration
        if( nameExists( name ) ) {
            return false;
        }
    } else {
        if( skip_to_description ) {
            cc_uistate.set_initial_tab( CHARCREATOR_SUMMARY );
        } else {
            cc_uistate.set_initial_tab( CHARCREATOR_SCENARIO );
        }
        character_creator_ui ccui;
        cc_uistate.generation_type = type;
        ccui.display();
        if( cc_uistate.quit_to_main_menu ) {
            return false;
        }
    }
    save_template( _( "Last Character" ), pool );

    initialize( type );

    return true;
}

void Character::set_skills_from_hobbies( bool no_override )
{
    for( const profession *profession : hobbies ) {
        for( const profession::StartingSkill &e : profession->skills() ) {
            if( no_override ) {
                continue;
            }
            if( get_skill_level( e.first ) < e.second ) {
                set_skill_level( e.first, e.second );
            }
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

namespace char_creation
{
void draw_action_button( const std::string &button_text, const std::string &button_action )
{
    if( ImGui::Button( button_text.c_str() ) ) {
        cc_uistate.top_bar_button_action = button_action;
    }
}

// @param draw_effective_stats - if true, draws effective (current) stat; base stat is always drawn
void draw_character_stats( const Character &who, bool draw_effective_stats )
{

    auto display_stat_value = [&draw_effective_stats]( int effective_stat, int base_stat ) {
        if( draw_effective_stats ) {
            nc_color effective_stat_color = c_white;
            if( effective_stat > base_stat ) {
                effective_stat_color = c_light_green;
            } else if( effective_stat < base_stat ) {
                effective_stat_color = c_light_red;
            }
            cataimgui::draw_colored_text( colorize( string_format( "%s (%s)",
                                                    effective_stat, base_stat ), effective_stat_color ), c_light_gray );
        } else {
            cataimgui::draw_colored_text( string_format( "%d", base_stat ), c_light_gray );
        }
    };

    cataimgui::draw_colored_text( _( "Stats:" ), COL_HEADER );
    if( ImGui::BeginTable( "STAT_TABLE", 2,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX ) ) {
        ImGui::TableNextColumn();
        const int stat_count = static_cast<int>( character_stat::DUMMY_STAT );
        for( int i = 0; i < stat_count; i++ ) {
            cataimgui::draw_colored_text( string_format( "%s:",
                                          get_character_stat_name( i ) ), c_light_gray );
        }
        ImGui::NewLine();
        ImGui::TableNextColumn();
        display_stat_value( who.get_str(), who.get_str_base() );
        display_stat_value( who.get_dex(), who.get_dex_base() );
        display_stat_value( who.get_int(), who.get_int_base() );
        display_stat_value( who.get_per(), who.get_per_base() );

        ImGui::EndTable();
    }

    char_creation::draw_age( who );
    char_creation::draw_height( who );
    char_creation::draw_blood( who );
}

void draw_character_skills( const Character &who )
{
    cataimgui::draw_colored_text( _( "Skills:" ), COL_HEADER );
    if( ImGui::BeginTable( "SKILL_TABLE", 2,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX ) ) {
        std::vector<const Skill *> skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a,
        const Skill & b ) {
            const int level_a = who.get_skill_level_object( a.ident() ).exercised_level();
            const int level_b = who.get_skill_level_object( b.ident() ).exercised_level();
            return localized_compare( std::make_pair( -level_a, a.name() ),
                                      std::make_pair( -level_b, b.name() ) );
        } );

        for( const Skill *skill : skillslist ) {
            const skill_id checked_skill = skill->ident();
            int skill_level = who.get_skill_level( checked_skill );

            // Handle skills from professions
            //if (pool != pool_type::TRANSFER) {
            for( auto &prof_skill : who.prof->skills() ) {
                if( prof_skill.first == checked_skill ) {
                    skill_level += prof_skill.second;
                    break;
                }
            }
            //}

            // Handle skills from hobbies
            for( const profession *hobby : who.hobbies ) {
                for( auto &hobby_skill : hobby->skills() ) {
                    if( hobby_skill.first == checked_skill ) {
                        skill_level = std::max( skill_level, hobby_skill.second );
                        break;
                    }
                }
            }

            skill_level = std::clamp( skill_level, MIN_SKILL, MAX_SKILL );
            if( skill_level > 0 ) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( string_format( "%s:", skill->name() ), c_light_gray );
                ImGui::TableNextColumn();
                cataimgui::draw_colored_text( string_format( "%d", skill_level ), c_light_gray );
                //has_skills = true;
            }
        }
        ImGui::EndTable();
    }
}

void draw_starting_vehicle( const Character &who )
{
    const vproto_id scen_veh = get_scenario()->vehicle();
    const vproto_id prof_veh = who.prof->vehicle();

    std::string vehicle_header = _( "Starting vehicle: " );
    std::string vehicle_details = _( "None!" );
    if( scen_veh ) {
        vehicle_header = _( "Starting vehicle (scenario):" );
        vehicle_details = scen_veh->name.translated();
    } else if( prof_veh ) {
        vehicle_header = _( "Starting vehicle (profession): " );
        vehicle_details = prof_veh->name.translated();
    }
    cataimgui::draw_colored_text( vehicle_header, COL_HEADER );
    cataimgui::draw_colored_text( vehicle_details, COL_NOTE_MINOR );
}

void draw_character_proficiencies( const Character &who )
{
    // Load in proficiencies from profession and hobbies
    std::vector<proficiency_id> prof_proficiencies = who.prof->proficiencies();
    const std::vector<proficiency_id> &known_proficiencies = who._proficiencies->known_profs();
    prof_proficiencies.insert( prof_proficiencies.end(), known_proficiencies.begin(),
                               known_proficiencies.end() );
    for( const profession *profession : who.hobbies ) {
        for( const proficiency_id &proficiency : profession->proficiencies() ) {
            // Do not add duplicate proficiencies
            if( std::find( prof_proficiencies.begin(), prof_proficiencies.end(),
                           proficiency ) == prof_proficiencies.end() ) {
                prof_proficiencies.push_back( proficiency );
            }
        }
    }

    cataimgui::draw_colored_text( _( "Proficiencies:" ), COL_HEADER );
    if( prof_proficiencies.empty() ) {
        cataimgui::draw_colored_text( _( "None!" ), c_light_red );
    } else {

        for( const proficiency_id &prof : prof_proficiencies ) {
            cataimgui::draw_colored_text( prof->name(), c_light_gray );
        }
    }
}

void draw_character_traits( const Character &who )
{
    cataimgui::draw_colored_text( _( "Traits:" ), COL_HEADER );
    trait_group::Trait_list traits_list = who.get_mutations_variants( false );
    std::sort( traits_list.begin(), traits_list.end(), trait_var_display_sort );
    for( const trait_and_var &character_trait : traits_list ) {
        const nc_color trait_color = character_trait.trait->get_display_color();
        cataimgui::draw_colored_text( character_trait.name(), trait_color );
    }
}

void draw_stat_details( avatar &u )
{
    std::string description_str;
    character_stat selected_stat = static_cast<character_stat>( cc_uistate.selected_stat_index );
    switch( selected_stat ) {
        case character_stat::STRENGTH: {
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

        case character_stat::DEXTERITY: {
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

        case character_stat::INTELLIGENCE: {
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

        case character_stat::PERCEPTION: {
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
        default:
            break;
    }
    cataimgui::draw_colored_text( description_str, ImGui::GetContentRegionAvail().x );
}

std::string stat_level_description( int stat_value )
{
    // Breakpoint values are largely borrowed from GAME_BALANCE.md.
    std::string description;
    if( stat_value >= 20 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "inhuman" );
    } else if( stat_value > 14 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "extraordinary" );
    } else if( stat_value > 12 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "top 1%" );
    } else if( stat_value > 10 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "top 10%" );
    } else if( stat_value > 8 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "above average" );
    } else if( stat_value == 8 ) { // special handling
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "average human" );
    } else if( stat_value > 6 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "below average" );
    } else if( stat_value > 4 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "impaired" );
    } else if( stat_value >= 0 ) {
        //~Description of a character's main stats. Should not exceed 18 characters of width.
        description = _( "incapacitated" );
    }

    return description;
}


std::string get_character_stat_header( int selected_stat_index )
{
    const int stat_value = cc_uistate.stats[selected_stat_index];
    return string_format( _( "%s: %d (%s)" ), get_character_stat_name( selected_stat_index ),
                          stat_value,
                          stat_level_description( stat_value ) );
}

const mutation_variant *variant_trait_selection_menu( const trait_id &cur_trait )
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
        if( pc.has_trait_variant( { cur_trait, pr.first } ) ) {
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

void draw_profession_header( const avatar &u )
{
    const std::vector<profession_id> &sorted_profs = cc_uistate.sorted_professions;
    const int selected_profession_index = cc_uistate.selected_profession_index;
    const profession &selected_profession = sorted_profs[selected_profession_index].obj();

    draw_colored_text_wrap( get_origin( selected_profession.src ), COL_NOTE_MINOR );

    draw_spacer();
    draw_colored_text_wrap( _( "Profession story:" ), COL_HEADER );
    if( !selected_profession.can_pick().success() ) {
        draw_colored_text_wrap( selected_profession.can_pick().str(), c_red );
    }
    draw_colored_text_wrap( selected_profession.description( u.male ), COL_NOTE_MINOR );
}

void draw_header( bool &drawn_anything, const std::string &title )
{
    if( !drawn_anything ) {
        drawn_anything = true;
    } else {
        draw_spacer();
    }
    draw_colored_text_wrap( title, COL_HEADER );
}

void draw_profession_addictions( bool &drawn_anything, const std::string &header,
                                 const profession &selected_profession )
{
    const auto prof_addictions = selected_profession.addictions();
    if( !prof_addictions.empty() ) {
        draw_header( drawn_anything, header );
        for( const addiction &a : prof_addictions ) {
            const char *format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
            draw_colored_text_wrap( string_format( format, a.type->get_name().translated(), a.intensity ),
                                    COL_NOTE_MINOR );
        }
    }
}

void draw_profession_traits( bool &drawn_anything, const std::string &header,
                             const profession &selected_profession )
{
    const std::vector<trait_and_var> &prof_traits = selected_profession.get_locked_traits();
    if( !prof_traits.empty() ) {
        draw_header( drawn_anything, header );
        for( const trait_and_var &t : prof_traits ) {
            draw_colored_text_wrap( t.name(), COL_NOTE_MINOR );
        }
    }
}

void draw_profession_proficiencies( bool &drawn_anything, const std::string &header,
                                    const profession &selected_profession )
{
    std::vector<proficiency_id> prof_proficiencies = selected_profession.proficiencies();
    if( !prof_proficiencies.empty() ) {
        draw_header( drawn_anything, header );
        for( const proficiency_id &prof : prof_proficiencies ) {
            draw_colored_text_wrap( prof->name(), COL_NOTE_MINOR );
        }
    }
}

void draw_profession_bionics( bool &drawn_anything, const std::string &header,
                              const profession &selected_profession )
{
    // active bionics shown first
    auto prof_CBMs = selected_profession.CBMs();
    if( !prof_CBMs.empty() ) {
        draw_header( drawn_anything, header );

        std::sort( std::begin( prof_CBMs ), std::end( prof_CBMs ), []( const bionic_id & a,
        const bionic_id & b ) {
            return a->activated && !b->activated;
        } );

        for( const auto &b : prof_CBMs ) {
            const bionic_data &cbm = b.obj();
            std::string format = "%s";
            if( cbm.activated && cbm.has_flag( json_flag_BIONIC_TOGGLED ) ) {
                format = _( "%s (toggled)" );
            } else if( cbm.activated ) {
                format = _( "%s (activated)" );
            }
            draw_colored_text_wrap( string_format( format, cbm.name.translated() ), COL_NOTE_MINOR );
        }
    }
}

void draw_profession_spells( bool &drawn_anything, const std::string &header,
                             const profession &selected_profession )
{
    if( !selected_profession.spells().empty() ) {
        draw_header( drawn_anything, header );
        for( const std::pair<const spell_id, int> &spell_pair : selected_profession.spells() ) {
            draw_colored_text_wrap( string_format( _( "%s level %d" ), spell_pair.first->name,
                                                   spell_pair.second ), COL_NOTE_MINOR );
        }
    }
}

void draw_profession_missions( bool &drawn_anything, const std::string &header,
                               const profession &selected_profession )
{
    if( !selected_profession.missions().empty() ) {
        draw_header( drawn_anything, header );
        for( mission_type_id mission_id : selected_profession.missions() ) {
            draw_colored_text_wrap( mission_type::get( mission_id )->tname(), COL_NOTE_MINOR );
        }
    }
}

void draw_profession_details()
{
    const std::vector<profession_id> &sorted_profs = cc_uistate.sorted_professions;
    const int selected_profession_index = cc_uistate.selected_profession_index;
    const profession &selected_profession = sorted_profs[selected_profession_index].obj();

    bool drawn_anything = false;

    if( !selected_profession.get_requirements().empty() ) {
        draw_header( drawn_anything, _( "Profession requirements:" ) );
        ret_val<void> can_pick_prof = selected_profession.can_pick();
        if( can_pick_prof.success() ) {
            std::vector<std::string> req_names;
            for( const achievement_id &req : selected_profession.get_requirements() ) {
                req_names.emplace_back( req->name().translated() );
            }
            draw_colored_text_wrap( string_format( n_gettext( _( "Completed \"%s\"\n" ), _( "Completed: %s\n" ),
                                                   req_names.size() ), enumerate_as_string( req_names ) ),
                                    COL_NOTE_MINOR );
        } else { // fail, can't pick so display ret_val's reason
            draw_colored_text_wrap( can_pick_prof.str(), c_red );
        }
    }

    draw_profession_addictions( drawn_anything, _( "Profession addictions:" ), selected_profession );
    draw_profession_traits( drawn_anything, _( "Profession traits:" ), selected_profession );

    // Profession martial art styles
    const auto prof_ma_known = selected_profession.ma_known();
    const auto prof_ma_choices = selected_profession.ma_choices();
    int ma_amount = selected_profession.ma_choice_amount;

    if( !prof_ma_known.empty() || !prof_ma_choices.empty() ) {
        draw_header( drawn_anything, _( "Profession martial arts:" ) );
        if( !prof_ma_known.empty() ) {
            draw_colored_text_wrap( _( "Known:" ), c_cyan );
            for( const matype_id &ma : prof_ma_known ) {
                const martialart &style = ma.obj();
                draw_colored_text_wrap( style.name.translated(), c_cyan );
            }
        }
        if( !prof_ma_known.empty() && !prof_ma_choices.empty() ) {
            ImGui::NewLine();
        }
        if( !prof_ma_choices.empty() ) {
            draw_colored_text_wrap( string_format( _( "Choose %d:" ), ma_amount ), c_cyan );
            for( const matype_id &ma : prof_ma_choices ) {
                const martialart &style = ma.obj();
                draw_colored_text_wrap( style.name.translated(), c_cyan );
            }
        }
    }

    // Profession skills
    const profession::StartingSkillList prof_skills = selected_profession.skills();
    if( !prof_skills.empty() ) {
        draw_header( drawn_anything, _( "Profession skills:" ) );
        for( const auto &sl : prof_skills ) {
            const char *format = pgettext( "set_profession_skill", "%1$s (%2$d)" );
            draw_colored_text_wrap( string_format( format, sl.first.obj().name(),
                                                   sl.second ), COL_NOTE_MINOR );
        }
    }

    draw_profession_bionics( drawn_anything, _( "Profession bionics:" ), selected_profession );
    draw_profession_proficiencies( drawn_anything, _( "Profession proficiencies:" ),
                                   selected_profession );

    std::vector<recipe_id> prof_recipe = selected_profession.recipes();
    if( !prof_recipe.empty() ) {
        draw_header( drawn_anything, _( "Profession recipes:" ) );
        for( const recipe_id &recipe : prof_recipe ) {
            draw_colored_text_wrap( recipe->result_name(), COL_NOTE_MINOR );
        }
    }

    if( !selected_profession.pets().empty() ) {
        draw_header( drawn_anything, _( "Profession pets:" ) );
        for( const mtype_id &elem : selected_profession.pets() ) {
            monster mon( elem ); // why not get name from mtype?
            draw_colored_text_wrap( mon.get_name(), COL_NOTE_MINOR );
        }
    }

    if( selected_profession.vehicle() ) {
        draw_header( drawn_anything, _( "Profession vehicle:" ) );
        vproto_id veh_id = selected_profession.vehicle();
        draw_colored_text_wrap( veh_id->name.translated(), COL_NOTE_MINOR );
    }

    draw_profession_spells( drawn_anything, _( "Profession spells:" ), selected_profession );
    draw_profession_missions( drawn_anything, _( "Profession missions:" ), selected_profession );

    std::optional<int> cash = selected_profession.starting_cash();
    if( cash.has_value() ) {
        draw_header( drawn_anything, _( "Profession money:" ) );
        draw_colored_text_wrap( format_money( cash.value() ), COL_NOTE_MINOR );
    }
}

void draw_profession_inventory_items( const std::string &category,
                                      const std::vector<std::string> &item_names )
{
    draw_colored_text_wrap( category, c_cyan );

    if( item_names.empty() ) {
        draw_colored_text_wrap( _( "None" ), COL_NOTE_MINOR );
    } else {
        for( const std::string &item_name : item_names ) {
            draw_colored_text_wrap( item_name, COL_NOTE_MINOR );
        }
    }
}

void draw_profession_inventory( const avatar &u )
{
    const std::vector<profession_id> &sorted_profs = cc_uistate.sorted_professions;
    const int selected_profession_index = cc_uistate.selected_profession_index;
    const profession &selected_profession = sorted_profs[selected_profession_index].obj();

    if( cc_uistate.cached_profession_inventory.empty() ) {
        cc_uistate.cached_profession_inventory = selected_profession.items( outfit, u.get_mutations() );
    }
    const std::list<item> &prof_items = cc_uistate.cached_profession_inventory;

    draw_colored_text_wrap( _( "Profession items:" ), COL_HEADER );
    if( prof_items.empty() ) {
        ImGui::NewLine();
        draw_colored_text_wrap( pgettext( "set_profession_item", "None" ), COL_NOTE_MINOR );
    } else {
        // TODO: If the item group is randomized *at all*, these will be different each time
        // and it won't match what you actually start with
        // TODO: Put like items together like the inventory does, so we don't have to scroll
        // through a list of a dozen forks.
        std::vector<std::string> assembled_wielded;
        std::vector<std::string> assembled_worn;
        std::vector<std::string> assembled_inventory;
        for( const item &it : prof_items ) {
            if( it.has_flag( json_flag_no_auto_equip ) ) { // NOLINT(bugprone-branch-clone)
                assembled_inventory.emplace_back( it.display_name() );
            } else if( it.has_flag( json_flag_auto_wield ) ) {
                assembled_wielded.emplace_back( it.display_name() );
            } else if( it.is_armor() ) {
                assembled_worn.emplace_back( it.display_name() );
            } else {
                assembled_inventory.emplace_back( it.display_name() );
            }
        }

        draw_profession_inventory_items( _( "Wielded:" ), assembled_wielded );
        draw_profession_inventory_items( _( "Worn:" ), assembled_worn );
        draw_profession_inventory_items( _( "Inventory:" ), assembled_inventory );
    }
}

void draw_hobby_header( const avatar &u )
{
    const profession &selected_hobby = *cc_uistate.get_selected_hobby();
    draw_colored_text_wrap( _( "Background story:" ), COL_HEADER );
    draw_colored_text_wrap( selected_hobby.description( u.male ), COL_NOTE_MINOR );
}

void draw_hobby_details()
{
    const profession &selected_hobby = *cc_uistate.get_selected_hobby();
    bool drawn_anything = false;

    draw_profession_addictions( drawn_anything, _( "Background addictions:" ), selected_hobby );
    draw_profession_traits( drawn_anything, _( "Background traits:" ), selected_hobby );

    // Background skills
    const profession::StartingSkillList prof_skills = selected_hobby.skills();
    if( !prof_skills.empty() ) {
        draw_header( drawn_anything, _( "Background skill experience:" ) );
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
            draw_colored_text_wrap( string_format( "%s (%s)", skill.name(),
                                                   skill_degree ), COL_NOTE_MINOR );
        }
    }

    draw_profession_proficiencies( drawn_anything, _( "Background proficiencies:" ), selected_hobby );
    draw_profession_bionics( drawn_anything, _( "Background bionics:" ), selected_hobby );
    draw_profession_spells( drawn_anything, _( "Background spells:" ), selected_hobby );
    draw_profession_missions( drawn_anything, _( "Background missions:" ), selected_hobby );
}

void draw_hobby_selected( const avatar &u )
{
    cataimgui::draw_colored_text( _( "Backgrounds Selected:" ), COL_HEADER );
    for( const profession *hobby : u.hobbies ) {
        cataimgui::draw_colored_text( hobby->gender_appropriate_name( u.male ), COL_NOTE_MINOR );
    }
}

void draw_skill_details( const avatar &u,
                         const std::map<skill_id, int> &prof_skills, skill_id currentSkill )
{
    draw_colored_text_wrap( _( "Your skill level grants access to the following recipes:" ), c_white );
    draw_spacer();

    std::string assembled;
    // We want recipes from profession skills displayed, but
    // without boosting the skills. Copy the skills, and boost the copy
    SkillLevelMap with_prof_skills = u.get_all_skills();
    for( const auto &sk : prof_skills ) {
        with_prof_skills.mod_skill_level( sk.first, sk.second );
    }

    //TODO: this recipe list is not easily navigable, put it in a searchable list
    std::map<std::string, std::vector<std::pair<std::string, int> > > recipes;
    for( const auto &e : recipe_dict ) {
        const recipe &r = e.second;
        if( r.is_practice() || r.has_flag( "SECRET" ) ) {
            continue;
        }
        //Find out if the current skill and its level is in the requirement list
        auto req_skill = r.required_skills.find( currentSkill );
        int skill = req_skill != r.required_skills.end() ? req_skill->second : 0;
        bool would_autolearn_recipe =
            recipe_dict.all_autolearn().count( &r ) &&
            with_prof_skills.meets_skill_requirements( r.autolearn_requirements );

        if( !would_autolearn_recipe && !r.never_learn &&
            ( r.skill_used == currentSkill || skill > 0 ) &&
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
            draw_colored_text_wrap( rec_temp, c_brown );
        } else {
            draw_colored_text_wrap( string_format( "[%s] %s", elem.first, rec_temp ), c_brown );
        }
    }
    if( recipes.empty() ) {
        draw_colored_text_wrap( _( "None!" ), c_light_red );
    }
}

void draw_scenario_details( const avatar &u )
{
    const scenario *current_scenario = cc_uistate.get_selected_scenario();
    std::string assembled;

    draw_colored_text_wrap( get_origin( current_scenario->src ), COL_NOTE_MINOR );
    draw_spacer();

    char_creation::draw_time_cataclysm_start();
    char_creation::draw_time_game_start();
    draw_spacer();

    draw_colored_text_wrap( _( "Scenario Story:" ), COL_HEADER );
    draw_colored_text_wrap( current_scenario->description( u.male ), COL_NOTE_MINOR );
    draw_spacer();
    const std::optional<achievement_id> scenRequirement = current_scenario->get_requirement();

    if( scenRequirement.has_value() ||
        ( current_scenario->has_flag( "CITY_START" ) && !cities_enabled() ) ) {
        draw_colored_text_wrap( _( "Scenario Requirements:" ), COL_HEADER );
        if( current_scenario->has_flag( "CITY_START" ) && !cities_enabled() ) {
            draw_colored_text_wrap(
                _( "This scenario is not available in this world due to city size settings." ), c_red );
            draw_spacer();
        }
        if( scenRequirement.has_value() ) {
            ret_val<void> can_pick_scenario = current_scenario->can_pick();
            if( can_pick_scenario.success() ) {
                draw_colored_text_wrap(
                    string_format( _( "Completed \"%s\"" ), scenRequirement.value()->name() ), c_green );
            } else { // fail, can't pick so display ret_val's reason
                draw_colored_text_wrap( can_pick_scenario.str(), c_red );
            }
            draw_spacer();
        }
    }

    draw_colored_text_wrap( _( "Scenario Professions:" ), COL_HEADER );
    draw_colored_text_wrap( string_format( _( "%s, default: " ), current_scenario->prof_count_str() ),
                            COL_NOTE_MINOR );

    profession_sorter psorter;
    const auto permitted = current_scenario->permitted_professions();
    const auto default_prof = *std::min_element( permitted.begin(), permitted.end(), psorter );

    draw_colored_text_wrap( default_prof->gender_appropriate_name( u.male ), COL_NOTE_MINOR );

    draw_spacer();
    draw_colored_text_wrap( _( "Scenario Location:" ), COL_HEADER );
    draw_colored_text_wrap( string_format( _( "%s (%d locations, %d variants)" ),
                                           current_scenario->start_name(),
                                           current_scenario->start_location_count(),
                                           current_scenario->start_location_targets_count() ), COL_NOTE_MINOR );
    draw_spacer();

    if( current_scenario->vehicle() ) {
        draw_colored_text_wrap( _( "Scenario Vehicle:" ), COL_HEADER );
        draw_colored_text_wrap( current_scenario->vehicle()->name.translated(), COL_NOTE_MINOR );
        draw_spacer();
    }

    if( !current_scenario->missions().empty() ) {
        draw_colored_text_wrap( _( "Scenario missions:" ), COL_HEADER );
        for( mission_type_id mission_id : current_scenario->missions() ) {
            draw_colored_text_wrap( mission_type::get( mission_id )->tname(), COL_NOTE_MINOR );
        }
        draw_spacer();
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
                draw_colored_text_wrap( _( "Scenario Flags:" ), COL_HEADER );
                flag_header_added = true;
            }
            draw_colored_text_wrap( std::get<1>( flag_pair ), COL_NOTE_MINOR );
        }
    }
}

enum description_selector {
    NAME,
    GENDER,
    OUTFIT,
    HEIGHT,
    AGE,
    BLOOD,
    LOCATION
};

void draw_name( const avatar &you, bool no_name_entered )
{
    std::string displayed_player_name;
    const nc_color special_case_color = c_light_gray;

    if( no_name_entered ) {
        displayed_player_name = colorize( _( "--- NO NAME ENTERED ---" ), special_case_color );
    } else if( you.name.empty() ) {
        displayed_player_name = colorize( _( "--- RANDOM NAME ---" ), special_case_color );
    } else {
        displayed_player_name = you.name;
    }
    draw_action_button( _( "Name:" ), "CHANGE_NAME" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( displayed_player_name, c_white );
}

std::string get_gender_string( bool male )
{
    return male ? colorize( _( "Male" ), c_light_cyan ) : colorize( _( "Female" ), c_pink );
}

void draw_gender( const avatar &you )
{
    draw_action_button( _( "Gender:" ), "CHANGE_GENDER" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( get_gender_string( you.male ), c_white );
}

void draw_outfit()
{
    draw_action_button( _( "Outfit:" ), "CHANGE_OUTFIT" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( get_gender_string( outfit ), c_white );
}

void draw_height( const Character &you )
{
    draw_action_button( _( "Height:" ), "CHANGE_HEIGHT" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( you.height_string(), c_light_gray );
}

void draw_age( const Character &you )
{
    draw_action_button( _( "Age:" ), "CHANGE_AGE" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( you.age_string( get_scenario()->start_of_game() ), c_light_gray );
}

void draw_blood( const Character &you )
{
    draw_action_button( _( "Blood type:" ), "CHANGE_BLOOD_TYPE" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( io::enum_to_string( you.my_blood_type ) +
                                  ( you.blood_rh_factor ? "+" : "-" ), c_light_gray );
}

void draw_scenario_profession( const avatar &you )
{
    draw_colored_text_wrap( string_format( _( "Scenario: %s  Profession: %s" ),
                                           colorize( get_scenario()->gender_appropriate_name( you.male ), c_light_gray ),
                                           colorize( you.prof->gender_appropriate_name( you.male ), c_light_gray ) ), c_white );
}

void draw_time_cataclysm_start()
{
    draw_action_button( _( "Start of cataclysm:" ), "CHANGE_START_OF_CATACLYSM" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( to_string( get_scenario()->start_of_cataclysm() ), c_light_gray );
}

void draw_time_game_start()
{
    draw_action_button( _( "Start of game:" ), "CHANGE_START_OF_GAME" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( to_string( get_scenario()->start_of_game() ), c_light_gray );
}

void draw_location( const avatar &you )
{
    std::string random_start_location_text = string_format( n_gettext(
                "<color_red>* Random location *</color> (<color_white>%d</color> variant)",
                "<color_red>* Random location *</color> (<color_white>%d</color> variants)",
                get_scenario()->start_location_targets_count() ), get_scenario()->start_location_targets_count() );

    if( get_scenario()->start_location_targets_count() == 1 ) {
        random_start_location_text = get_scenario()->start_location().obj().name();
    }

    char_creation::draw_action_button( _( "Starting location:" ), "CHOOSE_LOCATION" );

    // ::find will return empty location if id was not found. Debug msg will be printed too.
    cataimgui::draw_colored_text( string_format( n_gettext( "%s (%d variant)", "%s (%d variants)",
                                  you.start_location.obj().targets_count() ),
                                  you.start_location.obj().name(), you.start_location.obj().targets_count() ),
                                  you.random_start_location ? c_red : c_white );
}

void draw_starting_city( const avatar &you )
{
    char_creation::draw_action_button( _( "Starting city:" ), "CHOOSE_CITY" );
    if( you.starting_city.has_value() ) {
        cataimgui::draw_colored_text( you.starting_city->name, c_white );
    } else {
        cataimgui::draw_colored_text( _( "Random" ), c_light_gray );
    }
}

} // namespace char_creation

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
    //TODO: NPCs already get profession stuff assigned at least twice elsewhere causing issues and it all wants unifying (if not here this should be made an avatar::add_traits()
    if( !is_npc() ) {
        for( const trait_and_var &tr : prof->get_locked_traits() ) {
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
}

trait_id Character::random_good_trait()
{
    return get_random_trait( [this]( const mutation_branch & mb ) {
        return mb.points > 0 && ( mb.chargen_allow_npc || is_avatar() );
    } );
}

trait_id Character::random_bad_trait()
{
    return get_random_trait( [this]( const mutation_branch & mb ) {
        return mb.points < 0 &&
               ( ( !is_avatar() && mb.chargen_allow_npc ) ||
                 ( is_avatar() && mb.random_start_allowed ) );
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

    string_input_popup_imgui spop( FULL_SCREEN_WIDTH - utf8_width( title ) - 8 );
    spop.set_label( title );
    spop.set_description( desc );
    for( int character : fname_char_blacklist ) {
        spop.add_callback( callback_input( character ), []() {
            return true;
        } );
    }

    std::string ret = spop.query();
    if( spop.cancelled() ) {
        return std::nullopt;
    } else {
        return ret;
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
    profession_sorter psorter;
    const auto permitted = scen->permitted_professions();
    const auto default_prof = *std::min_element( permitted.begin(), permitted.end(), psorter );

    u.random_start_location = true;
    u.set_str_base( 8 );
    u.set_dex_base( 8 );
    u.set_int_base( 8 );
    u.set_per_base( 8 );
    set_scenario( scen );
    u.prof = &default_prof.obj();

    u.hobbies.clear();
    if( !scen->has_flag( flag_SKIP_DEFAULT_BACKGROUND ) ) {
        u.add_default_background();
    };
    u.clear_mutations();
    u.recalc_hp();
    u.empty_skills();
    u.add_traits();
}

character_creator_ui_impl::character_creator_ui_impl( character_creator_ui *parent ) :
    cataimgui::window( "",
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoNav )
{
    ui_parent = parent;
}

character_creator_ui::character_creator_ui()
{
    cc_callback = character_creator_callback( this );
}

std::shared_ptr<uilist> character_creator_ui::get_current_tab_uilist()
{
    return cc_uilist[static_cast<int>( cc_uistate.selected_tab )];
}

void character_creator_ui::set_current_tab_uilist( const std::shared_ptr<uilist> &new_uilist )
{
    cc_uilist[static_cast<int>( cc_uistate.selected_tab )] = new_uilist;
}

input_context &character_creator_ui::get_current_tab_input()
{
    return cc_inputs[static_cast<int>( cc_uistate.selected_tab )];
}

void character_creator_ui::set_current_tab_input( const input_context &new_input )
{
    cc_inputs[static_cast<int>( cc_uistate.selected_tab )] = new_input;
}

void character_creator_ui::upon_switching_tab()
{
    setup_avatar();
    update_uilist_entries();
}

void character_creator_ui::setup_input_context( input_context &cc_ictxt, bool quick_value_change )
{
    const bool allow_reroll = cc_uistate.generation_type == character_type::RANDOM;

    cc_ictxt.register_action( "NEXT_TAB" );
    cc_ictxt.register_action( "PREV_TAB" );
    cc_ictxt.register_action( "QUIT" );
    cc_ictxt.register_action( "HELP_KEYBINDINGS" );

    cc_ictxt.register_action( "SAVE_TEMPLATE" );
    cc_ictxt.register_action( "RANDOMIZE_CHAR_NAME" );
    cc_ictxt.register_action( "RANDOMIZE_CHAR_DESCRIPTION" );
    if( !MAP_SHARING::isSharing() && allow_reroll ) {
        cc_ictxt.register_action( "REROLL_CHARACTER" );
        cc_ictxt.register_action( "REROLL_CHARACTER_WITH_SCENARIO" );
    }
    cc_ictxt.register_action( "CHANGE_GENDER" );
    cc_ictxt.register_action( "CHANGE_OUTFIT" );
    cc_ictxt.register_action( "CHANGE_NAME" );
    cc_ictxt.register_action( "CHANGE_AGE" );
    cc_ictxt.register_action( "CHANGE_HEIGHT" );
    cc_ictxt.register_action( "CHANGE_BLOOD_TYPE" );
    cc_ictxt.register_action( "CHANGE_START_OF_CATACLYSM" );
    cc_ictxt.register_action( "CHANGE_START_OF_GAME" );
    cc_ictxt.register_action( "RESET_CALENDAR" );
    cc_ictxt.register_action( "SCROLL_INFOBOX_UP" );
    cc_ictxt.register_action( "SCROLL_INFOBOX_DOWN" );
    cc_ictxt.register_action( "TOGGLE_TOP_BAR" );
    if( get_option<bool>( "SELECT_STARTING_CITY" ) ) {
        cc_ictxt.register_action( "CHOOSE_CITY" );
    }
    cc_ictxt.register_action( "CHOOSE_LOCATION" );
    if( quick_value_change ) {
        cc_ictxt.register_action( "INCREASE_VALUE" );
        cc_ictxt.register_action( "DECREASE_VALUE" );
    }
}

static cataimgui::bounds uilist_reset_desired_bounds()
{
    return { 0, -1.0f, 0.33f * ImGui::GetMainViewport()->WorkSize.x, -1.0f };
}

void character_creator_ui::setup_new_uilist()
{
    std::shared_ptr<uilist> new_uilist = get_current_tab_uilist();

    if( !new_uilist ) {
        const avatar &u = get_avatar();

        input_context new_uilist_input;
        setup_input_context( new_uilist_input,
                             cc_uistate.selected_tab == CHARCREATOR_STATS || cc_uistate.selected_tab == CHARCREATOR_SKILLS );

        if( cc_uistate.selected_tab != CHARCREATOR_SUMMARY ) {
            set_current_tab_uilist( std::make_shared<uilist>() );
            new_uilist = get_current_tab_uilist();
            new_uilist->allow_additional = true;
            new_uilist->allow_cancel = false;
            new_uilist->desc_enabled = true;
            new_uilist->allow_disabled = true;
            new_uilist->hilight_disabled = true;
            new_uilist->size_to_all_categories = true;
            new_uilist->callback = &cc_callback;
            //snap to left edge of screen, position and height updated later
            new_uilist->desired_bounds = uilist_reset_desired_bounds();
            new_uilist->force_desired_bounds = true;
        }

        switch( cc_uistate.selected_tab ) {
            case CHARCREATOR_STATS: {
                new_uilist->filtering = false;
                break;
            }
            case CHARCREATOR_TRAITS: {
                cc_uistate.recalc_trait_list( u );
                std::vector<trait_id> &sorted_traits = cc_uistate.sorted_traits;
                new_uilist->set_category_filter( [&]( const uilist_entry & entry,
                const std::string & key )->bool {
                    const trait_id entry_trait = sorted_traits[entry.retval];
                    if( entry_trait.is_valid() )
                    {
                        if( key == CHARACTER_CREATOR_UILIST_ALL.translated() ) {
                            return true;
                        }
                        if( key == CHARACTER_CREATOR_TRAITS_POSITIVE.translated() && entry_trait->points > 0 ) {
                            return true;
                        }
                        if( key == CHARACTER_CREATOR_TRAITS_NEGATIVE.translated() && entry_trait->points < 0 ) {
                            return true;
                        }
                        if( key == CHARACTER_CREATOR_TRAITS_NEUTRAL.translated() && entry_trait->points == 0 ) {
                            return true;
                        }
                    }
                    return false;
                } );

                new_uilist->add_category( CHARACTER_CREATOR_UILIST_ALL.translated(),
                                          CHARACTER_CREATOR_UILIST_ALL.translated() );
                new_uilist->add_category( CHARACTER_CREATOR_TRAITS_POSITIVE.translated(),
                                          CHARACTER_CREATOR_TRAITS_POSITIVE.translated() );
                new_uilist->add_category( CHARACTER_CREATOR_TRAITS_NEGATIVE.translated(),
                                          CHARACTER_CREATOR_TRAITS_NEGATIVE.translated() );
                new_uilist->add_category( CHARACTER_CREATOR_TRAITS_NEUTRAL.translated(),
                                          CHARACTER_CREATOR_TRAITS_NEUTRAL.translated() );
                break;
            }
            case CHARCREATOR_SKILLS: {

                new_uilist->add_category( CHARACTER_CREATOR_UILIST_ALL.translated(),
                                          CHARACTER_CREATOR_UILIST_ALL.translated() );
                for( const SkillDisplayType &skill_category : SkillDisplayType::skillTypes ) {
                    const std::string category_key = skill_category.display_string();
                    const std::string first_word = string_split( category_key, ' ' ).front();
                    new_uilist->add_category( category_key, to_upper_case( first_word ) );
                }

                cc_uistate.sorted_skills = Skill::get_skills_sorted_by(
                []( const Skill & a, const Skill & b ) {
                    return localized_compare( a.name(), b.name() );
                } );
                std::vector<const Skill *> &sorted_skills = cc_uistate.sorted_skills;

                std::stable_sort( sorted_skills.begin(), sorted_skills.end(),
                []( const Skill * a, const Skill * b ) {
                    return a->display_category() < b->display_category();
                } );

                new_uilist->set_category_filter( [&]( const uilist_entry & entry,
                const std::string & key )->bool {
                    if( key == CHARACTER_CREATOR_UILIST_ALL.translated() )
                    {
                        return true;
                    }
                    const Skill *entry_skill = sorted_skills[entry.retval];
                    if( entry_skill )
                    {
                        return key == entry_skill->display_category()->display_string();
                    }
                    return false;
                } );
                break;
            }
            default:
                // do nothing; doesn't use a uilist
                break;
        }
        //add inputs from uilist to input_context
        if( new_uilist ) {
            new_uilist->register_uilist_inputs( new_uilist_input );
            new_uilist->setup();
        }
        set_current_tab_input( new_uilist_input );
    }
}

void character_creator_ui::update_uilist_entries()
{
    std::shared_ptr<uilist> menu = get_current_tab_uilist();
    const avatar &u = get_avatar();

    if( menu ) {
        menu->entries.clear();
    }

    switch( cc_uistate.selected_tab ) {
        case CHARCREATOR_SCENARIO: {
            cc_uistate.recalc_scenario_list( u );

            for( const scenario *scen : cc_uistate.sorted_scenarios ) {
                uilist_entry entry = get_uilist_entry( scen->gender_appropriate_name( u.male ) );
                entry.text_color = scen == cc_uistate.get_selected_scenario() ? COL_SELECTED : COL_NOT_SELECTED;
                entry.enabled = scen->can_pick().success();
                menu->addentry( entry );
            }
            break;
        }
        case CHARCREATOR_PROFESSION: {
            cc_uistate.recalc_profession_list( u );

            for( const profession_id prof_id : cc_uistate.sorted_professions ) {
                uilist_entry entry = get_uilist_entry( prof_id->gender_appropriate_name( u.male ) );
                entry.text_color = prof_id == cc_uistate.get_selected_profession() ? COL_SELECTED :
                                   COL_NOT_SELECTED;
                entry.enabled = prof_id->can_pick().success();
                menu->addentry( entry );
            }
            break;
        }
        case CHARCREATOR_BACKGROUND: {
            cc_uistate.recalc_hobby_list( u );

            for( const profession_id prof_id : cc_uistate.sorted_hobbies ) {
                uilist_entry entry = get_uilist_entry( prof_id->gender_appropriate_name( u.male ) );
                entry.text_color = u.hobbies.count( &*prof_id ) ? COL_SELECTED : COL_NOT_SELECTED;
                menu->addentry( entry );
            }
            break;
        }
        case CHARCREATOR_STATS: {
            cc_uistate.stats[static_cast<int>( character_stat::STRENGTH )] = u.get_str_base();
            cc_uistate.stats[static_cast<int>( character_stat::PERCEPTION )] = u.get_per_base();
            cc_uistate.stats[static_cast<int>( character_stat::INTELLIGENCE )] = u.get_int_base();
            cc_uistate.stats[static_cast<int>( character_stat::DEXTERITY )] = u.get_dex_base();

            const int stat_count = static_cast<int>( character_stat::DUMMY_STAT );
            for( int i = 0; i < stat_count; i++ ) {
                uilist_entry entry = get_uilist_entry( char_creation::get_character_stat_header( i ) );
                menu->addentry( get_uilist_entry( char_creation::get_character_stat_header( i ) ) );
            }
            break;
        }
        case CHARCREATOR_TRAITS: {

            const int trait_count = cc_uistate.sorted_traits.size();
            for( int i = 0; i < trait_count; i++ ) {
                trait_id current_trait = cc_uistate.sorted_traits[i];
                uilist_entry trait_entry = get_uilist_entry( current_trait->name() );
                trait_entry.retval = i;
                trait_entry.text_color = u.has_trait( current_trait ) ? COL_SELECTED : COL_NOT_SELECTED;
                trait_entry.enabled = !( u.has_conflicting_trait( current_trait ) ||
                                         get_scenario()->is_forbidden_trait( current_trait ) ||
                                         u.prof->is_forbidden_trait( current_trait ) );
                menu->addentry( trait_entry );
            }
            break;
        }
        case CHARCREATOR_SKILLS: {
            const int skill_count = cc_uistate.sorted_skills.size();
            for( int i = 0; i < skill_count; i++ ) {
                uilist_entry skill_entry = get_uilist_entry( get_skill_entry_text(
                                               cc_uistate.sorted_skills[i]->ident(), u ) );
                skill_entry.retval = i;
                menu->addentry( skill_entry );
            }
            break;
        }
        default:
            break;
    }
    if( menu ) {
        menu->desired_bounds = uilist_reset_desired_bounds();
        menu->filterlist();
        menu->setup();
    }
}

void character_creator_ui::setup_avatar()
{
    avatar &u = get_avatar();

    switch( cc_uistate.selected_tab ) {
        case CHARCREATOR_STATS:
            u.reset();
            u.reset_stats();
            u.recalc_speed_bonus();
            u.set_stored_kcal( u.get_healthy_kcal() );
            // Removes pollution of stats by modifications appearing inside reset_stats().
            // Is reset_stats() even necessary in this context?
            u.reset_bonuses();
            break;
        default:
            break;
    }
}

void character_creator_ui::update_uilist_position( ImVec2 new_position )
{
    std::shared_ptr<uilist> current_uilist = get_current_tab_uilist();
    current_uilist->desired_bounds = uilist_reset_desired_bounds();
    current_uilist->desired_bounds->y = new_position.y;
    current_uilist->desired_bounds->h = ImGui::GetContentRegionAvail().y;
    current_uilist->reposition();
    if( cc_uilist_current ) {
        cc_uilist_current->mark_resized();
    }
}

void character_creator_ui_impl::draw_controls()
{
    avatar &pc = get_avatar();

    const character_creator_tab switched_tab = cc_uistate.switched_tab;
    std::array<ImGuiTabItemFlags, CHARACTER_CREATOR_TAB_COUNT> tab_selected;
    tab_selected.fill( ImGuiTabItemFlags_None );

    // handle tab switch keyboard input
    if( switched_tab != character_creator_tab_LAST ) {
        tab_selected[static_cast<int>( switched_tab )] = ImGuiTabItemFlags_SetSelected;
        cc_uistate.selected_tab = switched_tab;
        cc_uistate.switched_tab = character_creator_tab_LAST;
    }

    auto check_new_tab = [this]( character_creator_tab new_tab ) {
        if( new_tab != cc_uistate.previous_tab ) {
            cc_uistate.selected_tab = new_tab;
            ui_parent->upon_switching_tab();
        }
        std::shared_ptr<uilist> current_uilist = ui_parent->get_current_tab_uilist();
        if( current_uilist ) {
            const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
            if( current_uilist->desired_bounds->y != cursor_pos.y ||
                current_uilist->desired_bounds->w != uilist_reset_desired_bounds().w ) {
                ui_parent->update_uilist_position( cursor_pos );
            }
        }
    };

    bool &top_bar_is_open = cc_uistate.top_bar_is_open;
    ImGui::SetNextItemOpen( top_bar_is_open );
    if( ( top_bar_is_open = ImGui::CollapsingHeader( string_format(
                                _( "General Info (Click or %s to %s)" ),
                                ui_parent->get_current_tab_input().get_desc( "TOGGLE_TOP_BAR" ),
                                top_bar_is_open ? _( "collapse" ) : _( "open" ) ).c_str() ) ) ) {
        draw_top_bar( pc );
    }

    if( ImGui::BeginTabBar( "CHARACTER_CREATOR_TABS" ) ) {
        if( ImGui::BeginTabItem( "SCENARIO", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_SCENARIO )] ) ) {
            check_new_tab( CHARCREATOR_SCENARIO );
            draw_scenarios();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "PROFESSION", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_PROFESSION )] ) ) {
            check_new_tab( CHARCREATOR_PROFESSION );
            draw_professions();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "BACKGROUND", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_BACKGROUND )] ) ) {
            check_new_tab( CHARCREATOR_BACKGROUND );
            draw_backgrounds();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "STATS", nullptr, tab_selected[static_cast<int>( CHARCREATOR_STATS )] ) ) {
            check_new_tab( CHARCREATOR_STATS );
            draw_stats();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "TRAITS", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_TRAITS )] ) ) {
            check_new_tab( CHARCREATOR_TRAITS );
            draw_traits();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "SKILLS", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_SKILLS )] ) ) {
            check_new_tab( CHARCREATOR_SKILLS );
            draw_skills();
            ImGui::EndTabItem();
        }
        if( ImGui::BeginTabItem( "SUMMARY", nullptr,
                                 tab_selected[static_cast<int>( CHARCREATOR_SUMMARY )] ) ) {
            check_new_tab( CHARCREATOR_SUMMARY );
            draw_summary();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    cc_uistate.previous_tab = cc_uistate.selected_tab;
}

void character_creator_ui_impl::draw_top_bar( const avatar &u ) const
{
    auto draw_separator = []() {
        ImGui::SameLine();
        draw_colored_text_wrap( " | ", c_white );
        ImGui::SameLine();
    };

    if( ImGui::BeginTable( "TOPBAR", 2, ImGuiTableFlags_BordersInnerV ) ) {
        ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed );
        ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthStretch );

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        const avatar &you = get_avatar();
        char_creation::draw_name( you, cc_uistate.no_name_entered );

        ImGui::TableSetColumnIndex( 1 );
        char_creation::draw_scenario_profession( you );

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex( 0 );
        char_creation::draw_gender( you );
        draw_separator();
        char_creation::draw_outfit();

        ImGui::TableSetColumnIndex( 1 );
        cataimgui::draw_colored_text( "Randomize:", c_white );
        ImGui::SameLine();
        char_creation::draw_action_button( _( "Name" ), "RANDOMIZE_CHAR_NAME" );
        ImGui::SameLine();
        char_creation::draw_action_button( _( "Description" ), "RANDOMIZE_CHAR_DESCRIPTION" );
        if( cc_uistate.generation_type == character_type::RANDOM ) {
            ImGui::SameLine();
            char_creation::draw_action_button( _( "All" ), "REROLL_CHARACTER" );
            ImGui::SameLine();
            char_creation::draw_action_button( _( "Keep Scenario" ), "REROLL_CHARACTER_WITH_SCENARIO" );
        }
        ImGui::EndTable();
    }

    if( cc_uistate.recalc_rating ) {
        cc_uistate.recalc_rating = false;
        cc_uistate.rating_string = player_difficulty::getInstance().difficulty_to_string( u );
    }
    draw_colored_text_wrap( cc_uistate.rating_string, c_white );

    char_creation::draw_action_button( _( "Save Template" ), "SAVE_TEMPLATE" );
    ImGui::SameLine();
    char_creation::draw_action_button( _( "Reset Calendar" ), "RESET_CALENDAR" );
    ImGui::SameLine();
    cataimgui::draw_colored_text( string_format(
                                      _( "Press <color_light_green>%s</color> to view and alter keybindings." ),
                                      ui_parent->get_current_tab_input().get_desc( "HELP_KEYBINDINGS" ) ) );
}

bool character_creator_ui::display()
{
    cc_uistate.reset();
    character_creator_ui_impl ccui( this );

    // setup all uilists/inputs
    character_creator_tab preserve_first_tab = cc_uistate.selected_tab;
    cc_uistate.selected_tab = CHARCREATOR_SCENARIO;
    for( int i = 0; i < CHARACTER_CREATOR_TAB_COUNT; i++ ) {
        setup_new_uilist();
        ++cc_uistate.selected_tab;
    }
    cc_uistate.selected_tab = preserve_first_tab;

    // load scenarios so that the past_games_info::ensure_loaded
    // redraw isn't called during uilist setup
    cc_uistate.recalc_scenario_list( get_avatar() );

    // set first tab
    upon_switching_tab();

    ui_manager::invalidate_all_ui_adaptors();
    cc_uistate.quit_to_main_menu = false;
    cc_uistate.finished_character_creator = false;

    while( !cc_uistate.finished_character_creator ) {

        ui_manager::redraw();
        std::shared_ptr<uilist> current_tab_uilist = get_current_tab_uilist();
        input_context &current_tab_input = get_current_tab_input();
        if( current_tab_uilist ) {
            cc_uilist_current = current_tab_uilist->create_or_get_ui();
            if( current_tab_uilist->query_setup() ) {
                current_tab_uilist->query_once( current_tab_input, 33 );
            }
        } else {
            cc_uilist_current.reset();
            handle_action( current_tab_input.handle_input( 33 ) );
        }
        if( !cc_uistate.top_bar_button_action.empty() ) {
            handle_action( cc_uistate.top_bar_button_action );
            cc_uistate.top_bar_button_action.clear();
        }
        if( cc_uistate.quit_to_main_menu ) {
            return false;
        }
    }
    return true;
}

void character_creator_ui_impl::draw_scenarios()
{
    const avatar &u = get_avatar();
    cc_uistate.recalc_scenario_list( u );
    const scenario *current_scenario = cc_uistate.get_selected_scenario();

    if( ImGui::BeginTable( "SCENARIO_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {
        if( current_scenario ) {
            std::string scenario_name = current_scenario->gender_appropriate_name( !u.male );
            if( current_scenario == get_scenario() ) {
                append_screen_reader_active( scenario_name );
            }
            setup_list_detail_ui( string_format( _( "Scenario: %s" ), scenario_name ) );
            char_creation::draw_scenario_details( u );
        } else {
            setup_list_detail_ui();
        }
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_professions()
{
    const avatar &u = get_avatar();
    cc_uistate.recalc_profession_list( u );

    if( ImGui::BeginTable( "PROFESSION_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {
        const profession_id &selected_profession = cc_uistate.get_selected_profession();

        if( !selected_profession.is_null() ) {
            std::string profession_name = selected_profession->gender_appropriate_name( u.male );
            if( u.prof->ident() == selected_profession ) {
                append_screen_reader_active( profession_name );
            }
            setup_list_detail_ui( string_format( _( "Profession: %s" ), profession_name ) );
            char_creation::draw_profession_header( u );
            draw_spacer();
            if( ImGui::BeginTable( "PROFESSION_COLUMNS", 2 ) ) {
                ImGui::TableNextColumn();
                char_creation::draw_profession_inventory( u );
                ImGui::NewLine();
                ImGui::TableNextColumn();
                char_creation::draw_profession_details();
                ImGui::EndTable();
            }
        } else {
            setup_list_detail_ui();
        }
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_backgrounds()
{
    const avatar &u = get_avatar();
    cc_uistate.recalc_hobby_list( u );
    cc_uistate.recalc_hobbies_taken_list( u );

    if( ImGui::BeginTable( "BACKGROUNDS_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {

        const profession_id selected_hobby = cc_uistate.get_selected_hobby();
        if( !selected_hobby.is_null() ) {
            std::string hobby_name = selected_hobby->gender_appropriate_name( u.male );
            if( u.hobbies.count( &*selected_hobby ) ) {
                append_screen_reader_active( hobby_name );
            }
            setup_list_detail_ui( string_format( _( "Background: %s" ), hobby_name ) );
            ImGui::NewLine();
            char_creation::draw_hobby_header( u );
            draw_spacer();
            if( ImGui::BeginTable( "BACKGROUNDS_COLUMNS", 2, ImGuiTableFlags_BordersInnerV,
                                   ImVec2( 0.0F, ImGui::GetContentRegionAvail().y ) ) ) {
                ImGui::TableNextColumn();
                char_creation::draw_hobby_details();
                ImGui::NewLine();
                ImGui::TableNextColumn();
                char_creation::draw_hobby_selected( u );
                ImGui::EndTable();
            }
        } else {
            setup_list_detail_ui();
        }
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_stats()
{
    if( ImGui::BeginTable( "STATS_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {
        setup_list_detail_ui( char_creation::get_character_stat_header( cc_uistate.selected_stat_index ) );
        ImGui::NewLine();
        char_creation::draw_stat_details( get_avatar() );
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_traits()
{
    const avatar &u = get_avatar();
    cc_uistate.recalc_trait_list( u );
    const trait_id selected_trait = cc_uistate.get_selected_trait();
    if( ImGui::BeginTable( "TRAITS_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {
        if( !selected_trait.is_null() ) {
            std::string trait_name = selected_trait->name();
            if( u.has_trait( selected_trait ) ) {
                append_screen_reader_active( trait_name );
            }
            setup_list_detail_ui( trait_name );
            draw_spacer();
            draw_colored_text_wrap( selected_trait->desc(), c_white );
        } else {
            setup_list_detail_ui();
        }
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_skills()
{
    if( ImGui::BeginTable( "SKILLS_MAIN", 2, CHARACTER_CREATOR_TABLE_FLAGS ) ) {
        const skill_id selected_skill = cc_uistate.get_selected_skill();
        if( !selected_skill.is_null() ) {
            const avatar &u = get_avatar();
            setup_list_detail_ui( remove_color_tags( get_skill_entry_text( selected_skill, u ) ) );
            draw_spacer();
            draw_colored_text_wrap( selected_skill->description(), c_white );
            draw_spacer();

            std::map<skill_id, int> prof_skills;
            const profession::StartingSkillList &pskills = u.prof->skills();
            std::copy( pskills.begin(), pskills.end(),
                       std::inserter( prof_skills, prof_skills.begin() ) );

            char_creation::draw_skill_details( u, prof_skills, selected_skill );
        } else {
            setup_list_detail_ui();
        }
        ImGui::EndTable();
    }
}

void character_creator_ui_impl::draw_summary()
{
    const avatar &u = get_avatar();
    const Character &who = get_player_character();
    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, { 20.0f, 0.0f } );
    if( ImGui::BeginTable( "DESCRIPTION_MAIN", 3, CHARACTER_CREATOR_TABLE_FLAGS |
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersInnerV ) ) {
        ImGui::TableNextColumn();

        char_creation::draw_character_stats( who, false );
        draw_spacer();
        char_creation::draw_character_skills( who );

        ImGui::NewLine();
        ImGui::TableNextColumn();
        char_creation::draw_character_traits( who );
        draw_spacer();
        char_creation::draw_character_proficiencies( who );
        draw_spacer();
        char_creation::draw_hobby_selected( u );

        ImGui::NewLine();
        ImGui::TableNextColumn();
        char_creation::draw_location( u );
        draw_spacer();
        if( get_option<bool>( "SELECT_STARTING_CITY" ) ) {
            char_creation::draw_starting_city( u );
            draw_spacer();
        }
        char_creation::draw_starting_vehicle( who );
        draw_spacer();
        bool dummy = true;
        char_creation::draw_profession_bionics( dummy, _( "Bionics" ), *who.prof );

        set_detail_scroll();

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

cataimgui::bounds character_creator_ui_impl::get_bounds()
{
    const ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    return { 0, 0, viewport.x, viewport.y };
}

void character_creator_uistate::recalc_scenario_list( const avatar &u )
{
    if( recalc_scenarios ) {
        std::vector<const scenario *> new_scenarios;
        for( const scenario &scen : scenario::get_all() ) {
            if( scen.scen_is_blacklisted() ) {
                continue;
            }
            new_scenarios.push_back( &scen );
        }
        scenario_sorter scen_sorter{ true, u.male, cities_enabled() };
        std::stable_sort( new_scenarios.begin(), new_scenarios.end(), scen_sorter );
        sorted_scenarios = new_scenarios;
        selected_scenario_index = 0;
        recalc_scenarios = false;
    }
}

void character_creator_uistate::recalc_profession_list( const avatar &u )
{
    if( recalc_professions ) {
        std::vector<profession_id> new_profs = get_scenario()->permitted_professions();
        profession_sorter prof_sorter { true, u.male };
        std::stable_sort( new_profs.begin(), new_profs.end(), prof_sorter );
        sorted_professions = new_profs;

        selected_profession_index = 0;
        recalc_professions = false;
    }
}

void character_creator_uistate::recalc_hobby_list( const avatar &u )
{
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
        profession_sorter prof_sorter { true, u.male };
        std::stable_sort( new_hobbies.begin(), new_hobbies.end(), prof_sorter );
        sorted_hobbies = new_hobbies;
        selected_hobby_index = 0;
        recalc_hobbies = false;
    }
}

void character_creator_uistate::recalc_hobbies_taken_list( const avatar &u )
{
    if( recalc_hobbies_taken ) {
        sorted_hobbies_taken.clear();
        for( const profession_id prof : sorted_hobbies ) {
            sorted_hobbies_taken.emplace_back( u.hobbies.count( &*prof ) == 0 );
        }
        recalc_hobbies_taken = false;
    }
}

bool character_creator_uistate::hobby_conflict_check( const avatar &u )
{
    // Do not allow selection of hobby if there's a trait conflict
    const profession_id &selected_hobby = get_selected_hobby();
    bool conflict_found = false;
    bool conflict_reason_found = false;
    for( const trait_and_var &new_trait : selected_hobby->get_locked_traits() ) {
        if( u.has_conflicting_trait( new_trait.trait ) ) {
            conflict_found = true;
            for( const trait_and_var &suspect : u.prof->get_locked_traits() ) {
                if( are_conflicting_traits( new_trait.trait, suspect.trait ) ) {
                    conflict_reason_found = true;
                    popup( _( "The trait [%1$s] conflicts with profession [%2$s]'s trait [%3$s]." ), new_trait.name(),
                           u.prof->gender_appropriate_name( u.male ), suspect.name() );
                }
            }
            for( const profession *hobby : u.hobbies ) {
                for( const trait_and_var &suspect : hobby->get_locked_traits() ) {
                    if( are_conflicting_traits( new_trait.trait, suspect.trait ) ) {
                        conflict_reason_found = true;
                        popup( _( "The trait [%1$s] conflicts with background [%2$s]'s trait [%3$s]." ), new_trait.name(),
                               hobby->gender_appropriate_name( u.male ), suspect.name() );
                    }
                }
            }
        }
    }
    if( conflict_found ) {
        if( !conflict_reason_found ) {
            popup( _( "A conflicting trait is preventing you from taking %s" ),
                   selected_hobby->gender_appropriate_name( u.male ) );
        }
        return false;
    }
    return true;
}

void character_creator_uistate::recalc_trait_list( const avatar &u )
{
    if( recalc_traits ) {
        sorted_traits.clear();
        for( const mutation_branch &traits_iter : mutation_branch::get_all() ) {
            // Don't list blacklisted traits
            if( mutation_branch::trait_is_blacklisted( traits_iter.id ) ) {
                continue;
            }

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
                sorted_traits.emplace_back( traits_iter.id );
            }
        }
        std::sort( sorted_traits.begin(), sorted_traits.end(), trait_display_nocolor_sort );
        selected_trait_index = 0;
        recalc_traits = false;
    }
}

void character_creator_uistate::set_initial_tab( character_creator_tab first_tab )
{
    selected_tab = switched_tab = previous_tab = first_tab;
}

const scenario *character_creator_uistate::get_selected_scenario()
{
    if( selected_scenario_index < 0 ) {
        return nullptr;
    }
    return sorted_scenarios[selected_scenario_index];
}

profession_id character_creator_uistate::get_selected_profession()
{
    if( selected_profession_index < 0 ) {
        return profession_id::NULL_ID();
    }
    return sorted_professions[selected_profession_index];
}

profession_id character_creator_uistate::get_selected_hobby()
{
    if( selected_hobby_index < 0 ) {
        return profession_id::NULL_ID();
    }
    return sorted_hobbies[selected_hobby_index];
}

trait_id character_creator_uistate::get_selected_trait()
{
    if( selected_trait_index < 0 ) {
        return trait_id::NULL_ID();
    }
    return sorted_traits[selected_trait_index];
}

skill_id character_creator_uistate::get_selected_skill()
{
    if( selected_skill_index < 0 ) {
        return skill_id::NULL_ID();
    }
    return sorted_skills[selected_skill_index]->ident();
}

void character_creator_uistate::reset()
{
    sorted_scenarios.clear();
    sorted_professions.clear();
    cached_profession_inventory.clear();
    sorted_hobbies.clear();
    sorted_traits.clear();
    sorted_skills.clear();
    sorted_hobbies_taken.clear();

    rating_string.clear();
    top_bar_button_action.clear();

    stats = { 8, 8, 8, 8 };

    selected_profession_index = 0;
    selected_scenario_index = 0;
    selected_hobby_index = 0;
    selected_stat_index = 0;
    selected_trait_index = 0;
    selected_skill_index = 0;

    recalc_rating = true;
    recalc_scenarios = true;
    recalc_professions = true;
    recalc_hobbies = true;
    recalc_hobbies_taken = true;
    recalc_traits = true;

    no_name_entered = false;
    scrolled_up = false;
    scrolled_down = false;
    quit_to_main_menu = false;
    finished_character_creator = false;
}

static int choose_location( const avatar &you )
{
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
    select_location.query();
    return select_location.ret;
}

bool character_creator_callback::key( const input_context &ctxt, const input_event &event,
                                      int, uilist * )
{
    const std::string &action = ctxt.input_to_action( event );
    if( !ui_parent->handle_action( action ) ) {
        return false;
    }
    return true;
}

bool character_creator_ui::handle_action( const std::string &action )
{
    avatar &you = get_avatar();

    auto mod_stat_base = [&you]( int mod_value ) {
        character_stat selected_stat = static_cast<character_stat>( cc_uistate.selected_stat_index );
        cc_uistate.stats[cc_uistate.selected_stat_index] = std::clamp(
                    cc_uistate.stats[cc_uistate.selected_stat_index] + mod_value,
                    CHARACTER_STAT_MIN, CHARACTER_STAT_MAX );
        set_stat_base( you, selected_stat, cc_uistate.stats[cc_uistate.selected_stat_index] );
    };
    auto mod_skill = [&you]( int mod_value ) {
        const skill_id selected_skill = cc_uistate.get_selected_skill();
        if( ( you.get_skill_level( selected_skill ) == MIN_SKILL && mod_value < 0 ) ||
            ( you.get_skill_level( selected_skill ) == MAX_SKILL && mod_value > 0 ) ) {
            return;
        }
        you.mod_skill_level( selected_skill, mod_value );
        you.mod_knowledge_level( selected_skill, mod_value );
    };

    if( action == "QUIT" && query_yn( _( "Return to main menu?" ) ) ) {
        cc_uistate.quit_to_main_menu = true;
    } else if( action == "PREV_TAB" ) {
        if( cc_uistate.selected_tab == CHARCREATOR_SCENARIO ) {
            if( query_yn( _( "Return to main menu?" ) ) ) {
                cc_uistate.quit_to_main_menu = true;
            }
        } else {
            --cc_uistate.selected_tab;
            cc_uistate.switched_tab = cc_uistate.selected_tab;
            upon_switching_tab();
        }
    } else if( action == "NEXT_TAB" ) {
        if( cc_uistate.selected_tab == CHARCREATOR_SUMMARY ) {
            if( you.name.empty() ) {
                if( query_yn( _( "Are you SURE you're finished?  Your name will be randomly generated." ) ) ) {
                    you.pick_name();
                    cc_uistate.finished_character_creator = true;
                }
            } else {
                if( query_yn( _( "Are you SURE you're finished?" ) ) ) {
                    cc_uistate.finished_character_creator = true;
                }
            }
        } else {
            ++cc_uistate.selected_tab;
            cc_uistate.switched_tab = cc_uistate.selected_tab;
            upon_switching_tab();
        }
    } else if( action == "SCROLL_INFOBOX_UP" ) {
        cc_uistate.scrolled_up = true;
    } else if( action == "SCROLL_INFOBOX_DOWN" ) {
        cc_uistate.scrolled_down = true;
    } else if( action == "TOGGLE_TOP_BAR" ) {
        cc_uistate.top_bar_is_open = !cc_uistate.top_bar_is_open;
    } else if( action == "SAVE_TEMPLATE" ) {
        if( const auto name = query_for_template_name() ) {
            you.save_template( *name, pool_type::FREEFORM );
        }
    } else if( action == "RANDOMIZE_CHAR_NAME" ) {
        // Don't allow random names when sharing maps.
        // We don't need to check at the top as you won't be able to edit the name
        if( !MAP_SHARING::isSharing() ) {
            you.pick_name();
        }
    } else if( action == "RANDOMIZE_CHAR_DESCRIPTION" ) {
        bool gender_selection = one_in( 2 );
        you.male = gender_selection;
        if( one_in( 10 ) ) {
            outfit = !gender_selection;
        } else {
            outfit = gender_selection;
        }
        if( !MAP_SHARING::isSharing() ) {
            you.pick_name();
        }
        you.set_base_age( rng( CHARACTER_AGE_MIN, CHARACTER_AGE_MAX ) );
        you.randomize_height();
        you.randomize_blood();
        you.randomize_heartrate();
    } else if( action == "REROLL_CHARACTER" ) {
        you.randomize( true );
    } else if( action == "REROLL_CHARACTER_WITH_SCENARIO" ) {
        you.randomize( false );
    } else if( action == "CHANGE_GENDER" ) {
        you.male = !you.male;
    } else if( action == "CHANGE_OUTFIT" ) {
        outfit = !outfit;
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
        const int selected_location_index = choose_location( you );
        if( selected_location_index == RANDOM_START_LOC_ENTRY ) {
            you.random_start_location = true;
        } else if( selected_location_index >= 0 ) {
            for( const start_location &loc : start_locations::get_all() ) {
                if( loc.id.id().to_i() == selected_location_index ) {
                    you.random_start_location = false;
                    you.start_location = loc.id;
                    break;
                }
            }
        }
    } else if( action == "CHANGE_NAME" ) {
        string_input_popup_imgui popup( 60, you.name );
        popup.set_description( _( "Enter name.  Cancel to delete all." ) );
        popup.set_max_input_length( NAME_CHARACTER_LIMIT );
        you.name = popup.query();
    } else if( action == "CHANGE_AGE" ) {
        const int age_current = you.base_age();
        number_input_popup<int> age_query( 0, age_current,
                                           string_format( _( "Enter age in years.  Minimum %d, maximum %d" ),
                                                   CHARACTER_AGE_MIN, CHARACTER_AGE_MAX ) );
        int age_result = age_query.query();
        if( age_result != age_current ) {
            you.set_base_age( clamp( age_result, CHARACTER_AGE_MIN, CHARACTER_AGE_MAX ) );
        }
    } else if( action == "CHANGE_HEIGHT" ) {

        const int height_current = you.base_height();
        const int min_allowed_height = Character::min_height();
        const int max_allowed_height = Character::max_height();

        number_input_popup<int> height_query( 0, height_current,
                                              string_format( _( "Enter height in centimeters.  Minimum %d, maximum %d" ),
                                                      min_allowed_height, max_allowed_height ) );
        int height_result = height_query.query();
        if( height_result != height_current ) {
            you.set_base_height( clamp( height_result, min_allowed_height, max_allowed_height ) );
        }
    } else if( action == "CHANGE_BLOOD_TYPE" ) {
        if( !you.blood_rh_factor ) {
            you.blood_rh_factor = true;
        } else {
            if( static_cast<blood_type>( static_cast<int>( you.my_blood_type ) + 1 ) != blood_type::num_bt ) {
                you.my_blood_type = static_cast<blood_type>( static_cast<int>( you.my_blood_type ) + 1 );
                you.blood_rh_factor = false;
            } else {
                you.my_blood_type = static_cast<blood_type>( 0 );
                you.blood_rh_factor = false;
            }
        }
    } else if( action == "INCREASE_VALUE" ) {
        if( cc_uistate.selected_tab == CHARCREATOR_SKILLS ) {
            mod_skill( 1 );
        } else if( cc_uistate.selected_tab == CHARCREATOR_STATS ) {
            mod_stat_base( 1 );
        }
        update_uilist_entries();
    } else if( action == "DECREASE_VALUE" ) {
        if( cc_uistate.selected_tab == CHARCREATOR_SKILLS ) {
            mod_skill( -1 );
        } else if( cc_uistate.selected_tab == CHARCREATOR_STATS ) {
            mod_stat_base( -1 );
        }
        update_uilist_entries();
    } else {
        return false;
    }
    cc_uistate.recalc_rating = true;
    cc_uistate.cached_profession_inventory.clear();
    return true;
}

void character_creator_callback::confirm( uilist *menu )
{
    avatar &u = get_avatar();
    int uilist_returned = menu->ret;

    if( menu->ret == -1 ) {
        uilist_returned = menu->selected;
        if( uilist_returned == -1 ) {
            return;
        }
    }

    switch( cc_uistate.selected_tab ) {
        case CHARCREATOR_SCENARIO: {
            cc_uistate.selected_scenario_index = uilist_returned;

            const scenario *selected_scenario = cc_uistate.get_selected_scenario();
            ret_val<void> can_pick = selected_scenario->can_pick();

            scenario_sorter sorter{ true, u.male, cities_enabled() };

            if( !can_pick.success() ) {
                popup( can_pick.str() );
                return;
            }

            if( selected_scenario->has_flag( "CITY_START" ) && !sorter.cities_enabled ) {
                return;
            }
            reset_scenario( u, selected_scenario );

            cc_uistate.recalc_professions = true;
            cc_uistate.recalc_hobbies = true;
            cc_uistate.recalc_traits = true;
            break;
        }
        case CHARCREATOR_PROFESSION: {
            cc_uistate.selected_profession_index = uilist_returned;

            const profession_id selected_profession = cc_uistate.get_selected_profession();
            ret_val<void> can_pick = cc_uistate.get_selected_profession()->can_pick();

            if( !can_pick.success() ) {
                popup( can_pick.str() );
                return;
            }

            // Remove traits from the previous profession
            for( const trait_and_var &old : u.prof->get_locked_traits() ) {
                u.toggle_trait_deps( old.trait );
            }

            u.prof = &*selected_profession;

            // Remove pre-selected traits that conflict
            // with the new profession's traits
            for( const trait_and_var &new_trait : selected_profession->get_locked_traits() ) {
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

            cc_uistate.recalc_hobbies = true;
            cc_uistate.recalc_traits = true;
            cc_uistate.cached_profession_inventory.clear();
            break;
        }
        case CHARCREATOR_BACKGROUND: {
            select( menu );
            if( !cc_uistate.hobby_conflict_check( u ) ) {
                return;
            }

            const profession *selected_hobby = &*cc_uistate.get_selected_hobby();
            // Toggle hobby
            bool enabling = false;
            if( u.hobbies.count( selected_hobby ) == 0 ) {
                // Add hobby, and decrement point cost
                u.hobbies.insert( selected_hobby );
                enabling = true;
            } else {
                // Remove hobby and refund point cost
                u.hobbies.erase( selected_hobby );
            }

            // Add or remove traits from hobby
            for( const trait_and_var &cur : selected_hobby->get_locked_traits() ) {
                const trait_id &trait = cur.trait;
                if( enabling ) {
                    if( !u.has_trait( trait ) ) {
                        u.toggle_trait_deps( trait );
                    }
                    continue;
                }
                int from_other_hobbies = u.prof->is_locked_trait( trait ) ? 1 : 0;
                for( const profession *hby : u.hobbies ) {
                    if( hby->ident() != selected_hobby->ident() && hby->is_locked_trait( trait ) ) {
                        from_other_hobbies++;
                    }
                }
                if( from_other_hobbies > 0 ) {
                    continue;
                }
                u.toggle_trait_deps( trait );
            }

            cc_uistate.recalc_traits = true;
            cc_uistate.recalc_hobbies_taken = true;
            cc_uistate.recalc_hobbies_taken_list( u );
            break;
        }
        case CHARCREATOR_STATS: {
            select( menu );
            const int selected_stat_index = cc_uistate.selected_stat_index;

            character_stat selected_stat = static_cast<character_stat>( selected_stat_index );
            const int stat_queried = cc_uistate.stats[selected_stat_index];
            number_input_popup<int> stat_query( 0, stat_queried,
                                                string_format( "Set new %s (between %d and %d):",
                                                        io::enum_to_full_string( selected_stat ),
                                                        CHARACTER_STAT_MIN, CHARACTER_STAT_MAX ) );
            int stat_queried_result = stat_query.query();
            const int stat_result_clamped = std::clamp( stat_queried_result, CHARACTER_STAT_MIN,
                                            CHARACTER_STAT_MAX );
            if( stat_result_clamped != stat_queried ) {
                set_stat_base( u, selected_stat, stat_result_clamped );
                cc_uistate.stats[selected_stat_index] = stat_result_clamped;
            }

            break;
        }
        case CHARCREATOR_TRAITS: {
            select( menu );
            int inc_type = 0;
            const trait_id cur_trait = cc_uistate.get_selected_trait();
            std::string variant;

            // Look through the profession bionics, and see if any of them conflict with this trait
            std::vector<bionic_id> cbms_blocking_trait = bionics_cancelling_trait( u.prof->CBMs(), cur_trait );
            const std::unordered_set<trait_id> conflicting_traits = u.get_conflicting_traits( cur_trait );

            if( u.has_trait( cur_trait ) ) {
                if( !cur_trait->variants.empty() ) {
                    const mutation_variant *rval = char_creation::variant_trait_selection_menu( cur_trait );
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
            } else {
                if( !cur_trait->variants.empty() ) {
                    const mutation_variant *rval = char_creation::variant_trait_selection_menu( cur_trait );
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
                u.toggle_trait_deps( cur_trait, variant );
            }
            break;
        }
        case CHARCREATOR_SKILLS: {
            select( menu );
            const skill_id skill_queried = cc_uistate.get_selected_skill();
            int previous_skill_level = u.get_skill_level( skill_queried );
            number_input_popup<int> skill_query( 0, previous_skill_level,
                                                 string_format( "Set new %s skill level (between %d and %d):",
                                                         skill_queried->name(), MIN_SKILL, MAX_SKILL ) );
            int skill_queried_result = skill_query.query();
            if( skill_queried_result != previous_skill_level ) {
                const int skill_result_clamped = std::clamp( skill_queried_result, MIN_SKILL, MAX_SKILL );
                u.set_skill_level( skill_queried, skill_result_clamped );
                u.set_knowledge_level( skill_queried, skill_result_clamped );
            }
            break;
        }
        default:
            break;
    }
    cc_uistate.recalc_rating = true;
    ui_parent->update_uilist_entries();
}

void character_creator_callback::select( uilist *menu )
{
    int menu_selected = menu->selected;
    switch( cc_uistate.selected_tab ) {
        case CHARCREATOR_SCENARIO:
            cc_uistate.selected_scenario_index = menu_selected;
            break;
        case CHARCREATOR_PROFESSION:
            cc_uistate.selected_profession_index = menu_selected;
            cc_uistate.cached_profession_inventory.clear();
            break;
        case CHARCREATOR_BACKGROUND:
            cc_uistate.selected_hobby_index = menu_selected;
            break;
        case CHARCREATOR_STATS:
            cc_uistate.selected_stat_index = menu_selected;
            break;
        case CHARCREATOR_TRAITS: {
            cc_uistate.selected_trait_index = menu_selected < 0 ? 0 : menu_selected;
            break;
        }
        case CHARCREATOR_SKILLS: {
            cc_uistate.selected_skill_index = menu_selected;
            break;
        }
        default:
            break;
    }
}

void character_creator_callback::refresh( uilist *menu )
{
    menu->footer_text = !menu->filter.empty() ? string_format( "Filter: %s",
                        menu->filter ) : std::string();
}
