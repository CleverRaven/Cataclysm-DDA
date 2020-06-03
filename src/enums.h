#pragma once
#ifndef CATA_SRC_ENUMS_H
#define CATA_SRC_ENUMS_H

template<typename T> struct enum_traits;

template<typename T>
constexpr inline int sgn( const T x )
{
    return x < 0 ? -1 : ( x > 0 ? 1 : 0 );
}

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
enum rule_state : int {
    RULE_NONE,
    RULE_WHITELISTED,
    RULE_BLACKLISTED
};

enum visibility_type {
    VIS_HIDDEN,
    VIS_CLEAR,
    VIS_LIT,
    VIS_BOOMER,
    VIS_DARK,
    VIS_BOOMER_DARK
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

enum special_game_id : int {
    SGAME_NULL = 0,
    SGAME_TUTORIAL,
    SGAME_DEFENSE,
    NUM_SPECIAL_GAMES
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
    UNDERWEAR,
    /* "Normal" layer, default if no flags set */
    REGULAR,
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
};

enum game_message_type : int {
    m_good,    /* something good happened to the player character, e.g. health boost, increasing in skill */
    m_bad,      /* something bad happened to the player character, e.g. damage, decreasing in skill */
    m_mixed,   /* something happened to the player character which is mixed (has good and bad parts),
                  e.g. gaining a mutation with mixed effect*/
    m_warning, /* warns the player about a danger. e.g. enemy appeared, an alarm sounds, noise heard. */
    m_info,    /* informs the player about something, e.g. on examination, seeing an item,
                  about how to use a certain function, etc. */
    m_neutral,  /* neutral or indifferent events which aren’t informational or nothing really happened e.g.
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
    game_message_params( const game_message_type message_type ) : type( message_type ),
        flags( gmf_none ) {}
    game_message_params( const game_message_type message_type,
                         const game_message_flags message_flags ) : type( message_type ), flags( message_flags ) {}

    /* Type of the message */
    game_message_type type;
    /* Flags pertaining to the message */
    game_message_flags flags;
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

#endif // CATA_SRC_ENUMS_H
