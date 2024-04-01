#pragma once
#ifndef CATA_SRC_ENUMS_H
#define CATA_SRC_ENUMS_H

#include <type_traits>

template<typename T> struct enum_traits;

template<typename T>
constexpr inline int sgn( const T x )
{
    return x < 0 ? -1 : ( x > 0 ? 1 : 0 );
}

enum class aim_exit : int {
    none = 0,
    okay,
    re_entry,
    last
};

template<>
struct enum_traits<aim_exit> {
    static constexpr aim_exit last = aim_exit::last;
};

// be explicit with the values
enum class aim_entry : int {
    START     = 0,
    VEHICLE   = 1,
    MAP       = 2,
    RESET     = 3,
    last
};

template<>
struct enum_traits<aim_entry> {
    static constexpr aim_entry last = aim_entry::last;
};

using I = std::underlying_type_t<aim_entry>;
inline constexpr aim_entry &operator++( aim_entry &lhs )
{
    lhs = static_cast<aim_entry>( static_cast<I>( lhs ) + 1 );
    return lhs;
}

inline constexpr aim_entry &operator--( aim_entry &lhs )
{
    lhs = static_cast<aim_entry>( static_cast<I>( lhs ) - 1 );
    return lhs;
}

inline constexpr aim_entry operator+( const aim_entry &lhs, const I &rhs )
{
    return static_cast<aim_entry>( static_cast<I>( lhs ) + rhs );
}

inline constexpr aim_entry operator-( const aim_entry &lhs, const I &rhs )
{
    return static_cast<aim_entry>( static_cast<I>( lhs ) - rhs );
}

enum class bionic_ui_sort_mode : int {
    NONE   = 0,
    POWER  = 1,
    NAME   = 2,
    INVLET = 3,
    nsort  = 4,
};

template<>
struct enum_traits<bionic_ui_sort_mode> {
    static constexpr bionic_ui_sort_mode last = bionic_ui_sort_mode::nsort;
};

// When bool is not enough. NONE, SOME or ALL
enum class trinary : int {
    NONE = 0,
    SOME = 1,
    ALL  = 2
};

enum class holiday : int {
    none = 0,
    new_year,
    easter,
    independence_day,
    halloween,
    thanksgiving,
    christmas,
    num_holiday
};

template<>
struct enum_traits<holiday> {
    static constexpr holiday last = holiday::num_holiday;
};

enum class temperature_flag : int {
    NORMAL = 0,
    HEATER,
    FRIDGE,
    FREEZER,
    ROOT_CELLAR
};

//Used for autopickup and safemode rules
enum class rule_state : int {
    NONE,
    WHITELISTED,
    BLACKLISTED
};

enum class visibility_type : int {
    HIDDEN,
    CLEAR,
    LIT,
    BOOMER,
    DARK,
    BOOMER_DARK
};

// Matching rules for comparing a string to an overmap terrain id.
enum class ot_match_type : int {
    // The provided string must completely match the overmap terrain id, including
    // linear direction suffixes for linear terrain types or rotation suffixes
    // for rotated terrain types.
    exact,
    // The provided string must completely match the base type id of the overmap
    // terrain id, which means that suffixes for rotation and linear terrain types
    // are ignored.
    type,
    // The provided string must completely match the base type id of the overmap
    // terrain id as well as the linear subtype.
    subtype,
    // The provided string must be a complete prefix (with additional parts delimited
    // by an underscore) of the overmap terrain id. For example, "forest" will match
    // "forest" or "forest_thick" but not "forestcabin".
    prefix,
    // The provided string must be contained within the overmap terrain id, but may
    // occur at the beginning, end, or middle and does not have any rules about
    // underscore delimiting.
    contains,
    num_ot_match_type
};

template<>
struct enum_traits<ot_match_type> {
    static constexpr ot_match_type last = ot_match_type::num_ot_match_type;
};

enum class special_game_type : int {
    NONE = 0,
    TUTORIAL,
    NUM_SPECIAL_GAME_TYPES
};

enum art_effect_passive : int {
    AEP_NULL = 0,
    // Good
    AEP_STR_UP, // Strength + 4
    AEP_DEX_UP, // Dexterity + 4
    AEP_PER_UP, // Perception + 4
    AEP_INT_UP, // Intelligence + 4
    AEP_ALL_UP, // All stats + 2
    AEP_SPEED_UP, // +20 speed
    AEP_PBLUE, // Reduces radiation
    AEP_SNAKES, // Summons friendly snakes when you're hit
    AEP_INVISIBLE, // Makes you invisible
    AEP_CLAIRVOYANCE, // See through walls
    AEP_SUPER_CLAIRVOYANCE, // See through walls to a great distance
    AEP_STEALTH, // Your steps are quieted
    AEP_EXTINGUISH, // May extinguish nearby flames
    AEP_GLOW, // Four-tile light source
    AEP_PSYSHIELD, // Protection from fear paralyze attack
    AEP_RESIST_ELECTRICITY, // Protection from electricity
    AEP_CARRY_MORE, // Increases carrying capacity by 200
    AEP_SAP_LIFE, // Killing non-zombie monsters may heal you
    AEP_FUN, // Slight passive morale
    // Splits good from bad
    AEP_SPLIT,
    // Bad
    AEP_HUNGER, // Increases hunger
    AEP_THIRST, // Increases thirst
    AEP_SMOKE, // Emits smoke occasionally
    AEP_EVIL, // Addiction to the power
    AEP_SCHIZO, // Mimicks schizophrenia
    AEP_RADIOACTIVE, // Increases your radiation
    AEP_MUTAGENIC, // Mutates you slowly
    AEP_ATTENTION, // Draws netherworld attention slowly
    AEP_STR_DOWN, // Strength - 3
    AEP_DEX_DOWN, // Dex - 3
    AEP_PER_DOWN, // Per - 3
    AEP_INT_DOWN, // Int - 3
    AEP_ALL_DOWN, // All stats - 2
    AEP_SPEED_DOWN, // -20 speed
    AEP_FORCE_TELEPORT, // Occasionally force a teleport
    AEP_MOVEMENT_NOISE, // Makes noise when you move
    AEP_BAD_WEATHER, // More likely to experience bad weather
    AEP_SICK, // Decreases health over time
    AEP_CLAIRVOYANCE_PLUS, // See through walls to a larger distance; not bad effect, placement to preserve old saves.

    NUM_AEPS
};

template<>
struct enum_traits<art_effect_passive> {
    static constexpr art_effect_passive last = art_effect_passive::NUM_AEPS;
};

enum artifact_natural_property {
    ARTPROP_NULL,
    ARTPROP_WRIGGLING, //
    ARTPROP_GLOWING, //
    ARTPROP_HUMMING, //
    ARTPROP_MOVING, //
    ARTPROP_WHISPERING, //
    ARTPROP_BREATHING, //
    ARTPROP_DEAD, //
    ARTPROP_ITCHY, //
    ARTPROP_GLITTERING, //
    ARTPROP_ELECTRIC, //
    ARTPROP_SLIMY, //
    ARTPROP_ENGRAVED, //
    ARTPROP_CRACKLING, //
    ARTPROP_WARM, //
    ARTPROP_RATTLING, //
    ARTPROP_SCALED,
    ARTPROP_FRACTAL,
    ARTPROP_MAX
};

enum class phase_id : int {
    PNULL,
    SOLID,
    LIQUID,
    GAS,
    PLASMA,
    num_phases
};

template<>
struct enum_traits<phase_id> {
    static constexpr phase_id last = phase_id::num_phases;
};

// Return the class an in-world object uses to interact with the world.
//   ex; if ( player.grab_type == object_type::VEHICLE ) { ...
//   or; if ( baseactor_just_shot_at.object_type() == object_type::NPC ) { ...
enum class object_type : int {
    NONE,      // Nothing, invalid.
    ITEM,      // item.h
    ACTOR,     // potential virtual base class, get_object_type() would return one of the types below
    PLAYER,  // player.h, npc.h
    NPC,   // nph.h
    MONSTER, // monster.h
    VEHICLE,   // vehicle.h
    TRAP,      // trap.h
    FIELD,     // field.h; field_entry
    TERRAIN,   // Not a real object
    FURNITURE, // Not a real object
    NUM_OBJECT_TYPES,
};

enum class liquid_source_type : int {
    INFINITE_MAP = 1,
    MAP_ITEM = 2,
    VEHICLE = 3,
    MONSTER = 4
};

enum class liquid_target_type : int {
    CONTAINER = 1,
    VEHICLE = 2,
    MAP = 3,
    MONSTER = 4
};

/**
 *  Possible layers that a piece of clothing/armor can occupy
 *
 *  Every piece of clothing occupies one distinct layer on the body-part that
 *  it covers.  This is used for example by @ref Character to calculate
 *  encumbrance values, @ref player to calculate time to wear/remove the item,
 *  and by @ref profession to place the characters' clothing in a sane order
 *  when starting the game.
 */
enum class layer_level : int {
    /* "Personal effects" layer, corresponds to PERSONAL flag */
    PERSONAL = 0,
    /* "Close to skin" layer, corresponds to SKINTIGHT flag. */
    SKINTIGHT,
    /* "Normal" layer, default if no flags set, also if NORMAL flag is used*/
    NORMAL,
    /* "Waist" layer, corresponds to WAIST flag. */
    WAIST,
    /* "Outer" layer, corresponds to OUTER flag. */
    OUTER,
    /* "Strapped" layer, corresponds to BELTED flag */
    BELTED,
    /* "Aura" layer, corresponds to AURA flag */
    AURA,
    /* Not a valid layer; used for C-style iteration through this enum */
    NUM_LAYER_LEVELS
};

template<>
struct enum_traits<layer_level> {
    static constexpr layer_level last = layer_level::NUM_LAYER_LEVELS;
};

inline layer_level &operator++( layer_level &l )
{
    l = static_cast<layer_level>( static_cast<int>( l ) + 1 );
    return l;
}

/** Possible reasons to interrupt an activity. */
enum class distraction_type : int {
    noise,
    pain,
    attacked,
    hostile_spotted_far,
    hostile_spotted_near,
    talked_to,
    asthma,
    motion_alarm,
    weather_change,
    portal_storm_popup,
    eoc,
    dangerous_field,
    hunger,
    thirst,
    temperature,
    mutation,
    oxygen,
    last,
};

template<>
struct enum_traits<distraction_type> {
    static constexpr distraction_type last = distraction_type::last;
};

enum game_message_type : int {
    m_good,    /* something good happened to the player character, e.g. health boost, increasing in skill */
    m_bad,      /* something bad happened to the player character, e.g. damage, decreasing in skill */
    m_mixed,   /* something happened to the player character which is mixed (has good and bad parts),
                  e.g. gaining a mutation with mixed effect*/
    m_warning, /* warns the player about a danger. e.g. enemy appeared, an alarm sounds, noise heard. */
    m_info,    /* informs the player about something, e.g. on examination, seeing an item,
                  about how to use a certain function, etc. */
    m_neutral,  /* neutral or indifferent events which aren't informational or nothing really happened e.g.
                  a miss, a non-critical failure. May also effect for good or bad effects which are
                  just very slight to be notable. This is the default message type. */

    m_debug, /* only shown when debug_mode is true */
    /* custom SCT colors */
    m_headshot,
    m_critical,
    m_grazing,
    num_game_message_type
};

template<>
struct enum_traits<game_message_type> {
    static constexpr game_message_type last = game_message_type::num_game_message_type;
};

enum game_message_flags {
    /* No specific game message flags */
    gmf_none = 0,
    /* Allow the message to bypass message cooldown. */
    gmf_bypass_cooldown = 1,
};

/** Structure allowing a combination of `game_message_type` and `game_message_flags`.
 */
struct game_message_params {
    // NOLINTNEXTLINE(google-explicit-constructor)
    game_message_params( const game_message_type message_type ) : type( message_type ),
        flags( gmf_none ) {}
    game_message_params( const game_message_type message_type,
                         const game_message_flags message_flags ) : type( message_type ), flags( message_flags ) {}

    /* Type of the message */
    game_message_type type;
    /* Flags pertaining to the message */
    game_message_flags flags;
};

struct social_modifiers {
    int lie = 0;
    int persuade = 0;
    int intimidate = 0;

    social_modifiers &operator+=( const social_modifiers &other ) {
        this->lie += other.lie;
        this->persuade += other.persuade;
        this->intimidate += other.intimidate;
        return *this;
    }
    bool empty() const {
        return this->lie != 0 || this->persuade != 0 || this->intimidate != 0;
    }
};

enum MULTITILE_TYPE {
    center,
    corner,
    edge,
    t_connection,
    end_piece,
    unconnected,
    open_,
    broken,
    num_multitile_types
};

enum class reachability_cache_quadrant : int {
    NE, SE, NW, SW
};

template<>
struct enum_traits<reachability_cache_quadrant> {
    static constexpr reachability_cache_quadrant last = reachability_cache_quadrant::SW;
    static constexpr int size = static_cast<int>( last ) + 1;

    inline static reachability_cache_quadrant quadrant( bool S, bool W ) {
        return static_cast<reachability_cache_quadrant>(
                   ( static_cast<int>( W ) << 1 ) | static_cast<int>( S )
               );
    }
};

enum class monotonically : int {
    constant,
    increasing,
    decreasing,
    unknown,
};

constexpr bool is_increasing( monotonically m )
{
    return m == monotonically::constant || m == monotonically::increasing;
}

constexpr bool is_decreasing( monotonically m )
{
    return m == monotonically::constant || m == monotonically::decreasing;
}

enum class character_type : int {
    CUSTOM,
    RANDOM,
    TEMPLATE,
    NOW,
    FULL_RANDOM,
};

enum class aggregate_type : int {
    FIRST,
    LAST,
    MIN,
    MAX,
    SUM,
    AVERAGE,
    num_aggregate_types
};

template<>
struct enum_traits<aggregate_type> {
    static constexpr aggregate_type last = aggregate_type::num_aggregate_types;
};

enum class link_state : int {
    no_link = 0,     // No connection, the default state
    needs_reeling,   // Cable has been disconnected and needs to be manually reeled in before it can be used again
    ups,             // Linked to a UPS the cable holder is holding
    solarpack,       // Linked to a solarpack the cable holder is wearing
    vehicle_port,    // Linked to a vehicle's cable ports / electrical controls or an appliance
    vehicle_battery, // Linked to a vehicle's battery or an appliance
    bio_cable,       // Linked to the cable holder's cable system bionic - source if connected to a vehicle, target otherwise
    vehicle_tow,     // Linked to a valid tow point on a vehicle - source if it's the towing vehicle, target if the towed one
    automatic,       // Use in link_to() to automatically set the type of connection based on the connected vehicle part. Is always true as a link_has_states() parameter.
    last
};
template<>
struct enum_traits<link_state> {
    static constexpr link_state last = link_state::last;
};

#endif // CATA_SRC_ENUMS_H
