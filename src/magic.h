#pragma once
#ifndef CATA_SRC_MAGIC_H
#define CATA_SRC_MAGIC_H

#include <functional>
#include <iosfwd>
#include <map>
#include <new>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "bodypart.h"
#include "damage.h"
#include "enum_bitset.h"
#include "event_subscriber.h"
#include "optional.h"
#include "point.h"
#include "sounds.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"

class Character;
class Creature;
class JsonIn;
class JsonObject;
class JsonOut;
class nc_color;
class spell;
class time_duration;
struct dealt_projectile_attack;
struct requirement_data;

namespace spell_effect
{
struct override_parameters;
} // namespace spell_effect

namespace cata
{
class event;
}  // namespace cata
template <typename E> struct enum_traits;

enum class spell_flag : int {
    PERMANENT, // items or creatures spawned with this spell do not disappear and die as normal
    IGNORE_WALLS, // spell's aoe goes through walls
    NO_PROJECTILE, // spell's original targeting area can be targeted through walls
    SWAP_POS, // a projectile spell swaps the positions of the caster and target
    HOSTILE_SUMMON, // summon spell always spawns a hostile monster
    HOSTILE_50, // summoned monster spawns friendly 50% of the time
    POLYMORPH_GROUP, // polymorph spell chooses a monster from a group
    FRIENDLY_POLY, // polymorph spell makes the monster friendly
    SILENT, // spell makes no noise at target
    LOUD, // spell makes extra noise at target
    VERBAL, // spell makes noise at caster location, mouth encumbrance affects fail %
    SOMATIC, // arm encumbrance affects fail % and casting time (slightly)
    NO_HANDS, // hands do not affect spell energy cost
    UNSAFE_TELEPORT, // teleport spell risks killing the caster or others
    NO_LEGS, // legs do not affect casting time
    CONCENTRATE, // focus affects spell fail %
    RANDOM_AOE, // picks random number between min+increment*level and max instead of normal behavior
    RANDOM_DAMAGE, // picks random number between min+increment*level and max instead of normal behavior
    RANDOM_DURATION, // picks random number between min+increment*level and max instead of normal behavior
    RANDOM_TARGET, // picks a random valid target within your range instead of normal behavior.
    RANDOM_CRITTER, // same as RANDOM_TARGET but ignores ground
    MUTATE_TRAIT, // overrides the mutate spell_effect to use a specific trait_id instead of a category
    WONDER, // instead of casting each of the extra_spells, it picks N of them and casts them (where N is std::min( damage(), number_of_spells ))
    PAIN_NORESIST, // pain altering spells can't be resisted (like with the deadened trait)
    NO_FAIL, // this spell cannot fail when you cast it
    WITH_CONTAINER, // items spawned with container
    SPAWN_GROUP, // spawn or summon from an item or monster group, instead of individual item/monster ID
    IGNITE_FLAMMABLE, // if spell effect area has any thing flammable, a fire will be produced
    MUST_HAVE_CLASS_TO_LEARN, // you can't learn the spell unless you already have the class.
    LAST
};

enum class magic_energy_type : int {
    hp,
    mana,
    stamina,
    bionic,
    none,
    last
};

enum class spell_target : int {
    ally,
    hostile,
    self,
    ground,
    none,
    item,
    field,
    num_spell_targets
};

enum class spell_shape : int {
    // circular blast area
    blast,
    // hits anything in a line from the caster to the target with a width
    line,
    // aoe is radius of the arc
    cone,
    num_shapes
};

template<>
struct enum_traits<magic_energy_type> {
    static constexpr magic_energy_type last = magic_energy_type::last;
};

template<>
struct enum_traits<spell_shape> {
    static constexpr spell_shape last = spell_shape::num_shapes;
};

template<>
struct enum_traits<spell_target> {
    static constexpr spell_target last = spell_target::num_spell_targets;
};

template<>
struct enum_traits<spell_flag> {
    static constexpr spell_flag last = spell_flag::LAST;
};

struct fake_spell {
    spell_id id;

    static const cata::optional<int> max_level_default;
    // max level this spell can be
    // if null pointer, spell can be up to its own max level
    cata::optional<int> max_level;

    static const int level_default;
    // level for things that need it
    int level = level_default;

    static const bool self_default;
    // target tripoint is source (true) or target (false)
    bool self = self_default;

    static const int trigger_once_in_default;
    // a chance to trigger the enchantment spells
    int trigger_once_in = trigger_once_in_default;
    // a message when the enchantment is triggered
    translation trigger_message;
    // a message when the enchantment is triggered and is on npc
    translation npc_trigger_message;

    fake_spell() = default;
    explicit fake_spell( const spell_id &sp_id, bool hit_self = false,
                         const cata::optional<int> &max_level = cata::nullopt ) : id( sp_id ),
        max_level( max_level ), self( hit_self ) {}

    bool operator==( const fake_spell &rhs ) const {
        return id == rhs.id &&
               max_level == rhs.max_level &&
               level == rhs.level &&
               self == rhs.self &&
               trigger_once_in == rhs.trigger_once_in &&
               trigger_message == rhs.trigger_message &&
               npc_trigger_message == rhs.npc_trigger_message;
    }

    // gets the spell with an additional override for minimum level (default 0)
    spell get_spell( int min_level_override = 0 ) const;

    void load( const JsonObject &jo );
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

class spell_events : public event_subscriber
{
    public:
        void notify( const cata::event & ) override;
};

class spell_type
{
    public:

        spell_type() = default;

        bool was_loaded = false;

        spell_id id;
        // spell name
        translation name;
        // spell description
        translation description;
        // spell message when cast
        translation message;
        // spell sound effect
        translation sound_description;
        skill_id skill;

        requirement_id spell_components;

        sounds::sound_t sound_type = sounds::sound_t::_LAST;
        bool sound_ambient = false;
        std::string sound_id;
        std::string sound_variant;
        // spell effect string. used to look up spell function
        std::string effect_name;
        std::function<void( const spell &, Creature &, const tripoint & )> effect;
        std::function<std::set<tripoint>( const spell_effect::override_parameters &params,
                                          const tripoint &source, const tripoint &target )> spell_area_function;
        // the spell shape found in the json
        spell_shape spell_area = spell_shape::line;
        // extra information about spell effect. allows for combinations for effects
        std::string effect_str;
        // list of additional "spell effects"
        std::vector<fake_spell> additional_spells;

        // if the spell has a field name defined, this is where it is
        cata::optional<field_type_id> field = cata::nullopt;
        // the chance one_in( field_chance ) that the field spawns at a tripoint in the area of the spell
        int field_chance = 0;
        // field intensity at spell level 0
        int min_field_intensity = 0;
        // increment of field intensity per level
        float field_intensity_increment = 0.0f;
        // maximum field intensity allowed
        int max_field_intensity = 0;
        // field intensity added to the map is +- ( 1 + field_intensity_variance ) * field_intensity
        float field_intensity_variance = 0.0f;

        // minimum damage this spell can cause
        int min_damage = 0;
        // amount of damage increase per spell level
        float damage_increment = 0.0f;
        // maximum damage this spell can cause
        int max_damage = 0;

        // minimum range of a spell
        int min_range = 0;
        // amount of range increase per spell level
        float range_increment = 0.0f;
        // max range this spell can achieve
        int max_range = 0;

        // minimum area of effect of a spell (radius)
        // 0 means the spell only affects the target
        int min_aoe = 0;
        // amount of area of effect increase per spell level (radius)
        float aoe_increment = 0.0f;
        // max area of effect of a spell (radius)
        int max_aoe = 0;

        // damage over time deals damage per turn

        // minimum damage over time
        int min_dot = 0;
        // increment per spell level
        float dot_increment = 0.0f;
        // max damage over time
        int max_dot = 0;

        // amount of time effect lasts

        // minimum time for effect in moves
        int min_duration = 0;
        // increment per spell level in moves
        // DoT is per turn, but increments can be smaller
        int duration_increment = 0;
        // max time for effect in moves
        int max_duration = 0;

        // amount of damage that is piercing damage
        // not added to damage stat

        // minimum pierce damage
        int min_pierce = 0;
        // increment of pierce damage per spell level
        float pierce_increment = 0.0f;
        // max pierce damage
        int max_pierce = 0;

        // base energy cost of spell
        int base_energy_cost = 0;
        // increment of energy cost per spell level
        float energy_increment = 0.0f;
        // max or min energy cost, based on sign of energy_increment
        int final_energy_cost = 0;

        // spell is restricted to being cast by only this class
        // if spell_class is empty, spell is unrestricted
        trait_id spell_class;

        // the difficulty of casting a spell
        int difficulty = 0;

        // max level this spell can achieve
        int max_level = 0;

        // base amount of time to cast the spell in moves
        int base_casting_time = 0;
        // increment of casting time per level
        float casting_time_increment = 0.0f;
        // max or min casting time
        int final_casting_time = 0;

        // Does leveling this spell lead to learning another spell?
        std::map<std::string, int> learn_spells;

        // what energy do you use to cast this spell
        magic_energy_type energy_source = magic_energy_type::none;

        damage_type dmg_type = damage_type::NONE;

        // list of valid targets enum
        enum_bitset<spell_target> valid_targets;

        std::set<mtype_id> targeted_monster_ids;

        // lits of bodyparts this spell applies its effect to
        body_part_set affected_bps;

        enum_bitset<spell_flag> spell_tags;

        static void load_spell( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
        void serialize( JsonOut &json ) const;
        /**
         * All spells in the game.
         */
        static const std::vector<spell_type> &get_all();
        static void check_consistency();
        static void reset_all();
        bool is_valid() const;
    private:
        // default values

        static const skill_id skill_default;
        static const requirement_id spell_components_default;
        static const translation message_default;
        static const translation sound_description_default;
        static const sounds::sound_t sound_type_default;
        static const bool sound_ambient_default;
        static const std::string sound_id_default;
        static const std::string sound_variant_default;
        static const std::string effect_str_default;
        static const cata::optional<field_type_id> field_default;
        static const int field_chance_default;
        static const int min_field_intensity_default;
        static const int max_field_intensity_default;
        static const float field_intensity_increment_default;
        static const float field_intensity_variance_default;
        static const int min_damage_default;
        static const float damage_increment_default;
        static const int max_damage_default;
        static const int min_range_default;
        static const float range_increment_default;
        static const int max_range_default;
        static const int min_aoe_default;
        static const float aoe_increment_default;
        static const int max_aoe_default;
        static const int min_dot_default;
        static const float dot_increment_default;
        static const int max_dot_default;
        static const int min_duration_default;
        static const int duration_increment_default;
        static const int max_duration_default;
        static const int min_pierce_default;
        static const float pierce_increment_default;
        static const int max_pierce_default;
        static const int base_energy_cost_default;
        static const float energy_increment_default;
        static const trait_id spell_class_default;
        static const magic_energy_type energy_source_default;
        static const damage_type dmg_type_default;
        static const int difficulty_default;
        static const int max_level_default;
        static const int base_casting_time_default;
        static const float casting_time_increment_default;
};

// functions for spell description
namespace spell_desc
{
bool casting_time_encumbered( const spell &sp, const Character &guy );
bool energy_cost_encumbered( const spell &sp, const Character &guy );
std::string enumerate_spell_data( const spell &sp );
} // namespace spell_desc

class spell
{
    private:
        friend class spell_events;
        // basic spell data
        spell_id type;

        // once you accumulate enough exp you level the spell
        int experience = 0;
        // returns damage type for the spell
        damage_type dmg_type() const;

        // alternative cast message
        translation alt_message;

        // minimum damage including levels
        int min_leveled_damage() const;
        int min_leveled_dot() const;
        // minimum aoe including levels
        int min_leveled_aoe() const;
        // minimum duration including levels (moves)
        int min_leveled_duration() const;

    public:
        spell() = default;
        explicit spell( spell_id sp, int xp = 0 );

        // sets the message to be different than the spell_type specifies
        void set_message( const translation &msg );

        static int exp_for_level( int level );
        // how much exp you need for the spell to gain a level
        int exp_to_next_level() const;
        // progress to the next level, expressed as a percent
        std::string exp_progress() const;
        // how much xp you have total
        int xp() const;
        // gain some exp
        void gain_exp( int nxp );
        void set_exp( int nxp );
        // how much xp you get if you successfully cast the spell
        int casting_exp( const Character &guy ) const;
        // modifier for gaining exp
        float exp_modifier( const Character &guy ) const;
        // level up!
        void gain_level();
        // gains a number of levels, or until max. 0 or less just returns early.
        void gain_levels( int gains );
        void set_level( int nlevel );
        // is the spell at max level?
        bool is_max_level() const;
        // what is the max level of the spell
        int get_max_level() const;

        spell_shape shape() const;
        // what is the intensity of the field the spell generates ( 0 if no field )
        int field_intensity() const;
        // how much damage does the spell do
        int damage() const;
        int damage_dot() const;
        damage_over_time_data damage_over_time( const std::vector<bodypart_str_id> &bps ) const;
        dealt_damage_instance get_dealt_damage_instance() const;
        dealt_projectile_attack get_projectile_attack( const tripoint &target,
                Creature &hit_critter ) const;
        damage_instance get_damage_instance() const;
        // calculate damage per second against a target
        float dps( const Character &caster, const Creature &target ) const;
        // how big is the spell's radius
        int aoe() const;
        std::set<tripoint> effect_area( const spell_effect::override_parameters &params,
                                        const tripoint &source, const tripoint &target ) const;
        std::set<tripoint> effect_area( const tripoint &source, const tripoint &target ) const;
        // distance spell can be cast
        int range() const;
        /**
         *  all of the tripoints the spell can be cast at.
         *  if the spell can't be cast through walls, does not return anything behind walls
         *  if the spell can't target the ground, can't target unseen locations, etc.
         */
        std::vector<tripoint> targetable_locations( const Character &source ) const;
        // how much energy does the spell cost
        int energy_cost( const Character &guy ) const;
        // how long does this spell's effect last
        int duration() const;
        time_duration duration_turns() const;
        // how often does the spell fail
        // based on difficulty, level of spell, spellcraft skill, intelligence
        float spell_fail( const Character &guy ) const;
        std::string colorized_fail_percent( const Character &guy ) const;
        // how long does it take to cast the spell
        int casting_time( const Character &guy, bool ignore_encumb = false ) const;
        // the requirement data for spell components. includes tools, items, and qualities.
        const requirement_data &components() const;
        bool has_components() const;
        // can the Character cast this spell?
        bool can_cast( const Character &guy ) const;
        // can the Character learn this spell?
        bool can_learn( const Character &guy ) const;
        // is this spell valid
        bool is_valid() const;
        // is the bodypart affected by the effect
        bool bp_is_affected( const bodypart_str_id &bp ) const;
        // check if the spell has a particular flag
        bool has_flag( const spell_flag &flag ) const;
        // check if the spell's class is the same as input
        bool is_spell_class( const trait_id &mid ) const;

        bool in_aoe( const tripoint &source, const tripoint &target ) const;

        // get spell id (from type)
        spell_id id() const;
        // get spell class (from type)
        trait_id spell_class() const;
        // get skill id
        skill_id skill() const;
        // get spell effect string (from type)
        std::string effect() const;
        // get spell effect_str data
        std::string effect_data() const;
        // get spell summon vehicle id
        vproto_id summon_vehicle_id() const;
        // name of spell (translated)
        std::string name() const;
        // description of spell (translated)
        std::string description() const;
        // spell message when cast (translated)
        std::string message() const;
        // energy source as a string (translated)
        std::string energy_string() const;
        // energy cost returned as a string
        std::string energy_cost_string( const Character &guy ) const;
        // current energy the Character has available as a string
        std::string energy_cur_string( const Character &guy ) const;
        // prints out a list of valid targets separated by commas
        std::string enumerate_targets() const;
        // returns the name string of all list of all targeted monster id
        //if targeted_monster_ids is empty, it returns an empty string
        std::string list_targeted_monster_names() const;

        std::string damage_string() const;
        std::string aoe_string() const;
        std::string duration_string() const;

        // magic energy source enum
        magic_energy_type energy_source() const;
        // the color that's representative of the damage type
        nc_color damage_type_color() const;
        std::string damage_type_string() const;
        // your level in this spell
        int get_level() const;
        // difficulty of the level
        int get_difficulty() const;

        // tries to create a field at the location specified
        void create_field( const tripoint &at ) const;

        int sound_volume() const;
        // makes a spell sound at the location
        void make_sound( const tripoint &target ) const;
        void make_sound( const tripoint &target, int loudness ) const;
        // heals the critter at the location, returns amount healed (Character heals each body part)
        int heal( const tripoint &target ) const;

        // casts the spell effect. returns true if successful
        void cast_spell_effect( Creature &source, const tripoint &target ) const;
        // goes through the spell effect and all of its internal spells
        void cast_all_effects( Creature &source, const tripoint &target ) const;
        // uses up the components in @guy's inventory
        void use_components( Character &guy ) const;
        // checks if a target point is in spell range
        bool is_target_in_range( const Creature &caster, const tripoint &p ) const;

        // is the target valid for this spell?
        bool is_valid_target( const Creature &caster, const tripoint &p ) const;
        bool is_valid_target( spell_target t ) const;
        bool target_by_monster_id( const tripoint &p ) const;

        // picks a random valid tripoint from @area
        cata::optional<tripoint> random_valid_target( const Creature &caster,
                const tripoint &caster_pos ) const;
};

class known_magic
{
    private:
        // list of spells known
        std::map<spell_id, spell> spellbook;
        // invlets assigned to spell_id
        std::map<spell_id, int> invlets;
        // the base mana a Character would start with
        int mana_base = 0;
        // current mana
        int mana = 0;
    public:
        // ignores all distractions when casting a spell when true
        bool casting_ignore = false;

        known_magic();

        void learn_spell( const std::string &sp, Character &guy, bool force = false );
        void learn_spell( const spell_id &sp, Character &guy, bool force = false );
        void learn_spell( const spell_type *sp, Character &guy, bool force = false );
        void forget_spell( const std::string &sp );
        void forget_spell( const spell_id &sp );
        // time in moves for the Character to memorize the spell
        int time_to_learn_spell( const Character &guy, const spell_id &sp ) const;
        int time_to_learn_spell( const Character &guy, const std::string &str ) const;
        bool can_learn_spell( const Character &guy, const spell_id &sp ) const;
        bool knows_spell( const std::string &sp ) const;
        bool knows_spell( const spell_id &sp ) const;
        // does the Character know a spell?
        bool knows_spell() const;
        // spells known by Character
        std::vector<spell_id> spells() const;
        // gets the spell associated with the spell_id to be edited
        spell &get_spell( const spell_id &sp );
        // opens up a ui that the Character can choose a spell from
        // returns the index of the spell in the vector of spells
        int select_spell( Character &guy );
        // get all known spells
        std::vector<spell *> get_spells();
        // directly get the character known spells
        std::map<spell_id, spell> &get_spellbook() {
            return spellbook;
        }
        // how much mana is available to use to cast spells
        int available_mana() const;
        // max mana vailable
        int max_mana( const Character &guy ) const;
        void mod_mana( const Character &guy, int add_mana );
        void set_mana( int new_mana );
        void update_mana( const Character &guy, float turns );
        // does the Character have enough energy to cast this spell?
        // not specific to mana
        bool has_enough_energy( const Character &guy, const spell &sp ) const;

        void on_mutation_gain( const trait_id &mid, Character &guy );
        void on_mutation_loss( const trait_id &mid );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        // returns false if invlet is already used
        bool set_invlet( const spell_id &sp, int invlet, const std::set<int> &used_invlets );
        void rem_invlet( const spell_id &sp );
    private:
        // gets length of longest spell name
        int get_spellname_max_width();
        // gets invlet if assigned, or -1 if not
        int get_invlet( const spell_id &sp, std::set<int> &used_invlets );
};

namespace spell_effect
{
// a struct available for override parameters.
// is a separate struct for later extensibility
// used for spell shapes to be lighter weight than passing the whole spell
// and allows for overrides for these values
struct override_parameters {
    int aoe_radius;
    int range;
    bool ignore_walls;

    explicit override_parameters( const spell &sp ) {
        aoe_radius = sp.aoe();
        range = sp.range();
        ignore_walls = sp.has_flag( spell_flag::IGNORE_WALLS );
    }
};

void teleport_random( const spell &sp, Creature &caster, const tripoint & );
void pain_split( const spell &, Creature &, const tripoint & );
void attack( const spell &sp, Creature &caster,
             const tripoint &epicenter );
void targeted_polymorph( const spell &sp, Creature &caster, const tripoint &target );

void area_pull( const spell &sp, Creature &caster, const tripoint &center );
void area_push( const spell &sp, Creature &caster, const tripoint &center );
void directed_push( const spell &sp, Creature &caster, const tripoint &target );

std::set<tripoint> spell_effect_blast( const override_parameters &params, const tripoint &,
                                       const tripoint &target );
std::set<tripoint> spell_effect_cone( const override_parameters &params, const tripoint &source,
                                      const tripoint &target );
std::set<tripoint> spell_effect_line( const override_parameters &params, const tripoint &source,
                                      const tripoint &target );

void spawn_ethereal_item( const spell &sp, Creature &, const tripoint & );
void recover_energy( const spell &sp, Creature &, const tripoint &target );
void spawn_summoned_monster( const spell &sp, Creature &caster, const tripoint &target );
void spawn_summoned_vehicle( const spell &sp, Creature &caster, const tripoint &target );
void translocate( const spell &sp, Creature &caster, const tripoint &target );
// adds a timed event to the caster only
void timed_event( const spell &sp, Creature &caster, const tripoint & );
void transform_blast( const spell &sp, Creature &caster, const tripoint &target );
void noise( const spell &sp, Creature &, const tripoint &target );
void vomit( const spell &sp, Creature &caster, const tripoint &target );
void explosion( const spell &sp, Creature &, const tripoint &target );
void flashbang( const spell &sp, Creature &caster, const tripoint &target );
void mod_moves( const spell &sp, Creature &caster, const tripoint &target );
void map( const spell &sp, Creature &caster, const tripoint & );
void morale( const spell &sp, Creature &caster, const tripoint &target );
void charm_monster( const spell &sp, Creature &caster, const tripoint &target );
void mutate( const spell &sp, Creature &caster, const tripoint &target );
void bash( const spell &sp, Creature &caster, const tripoint &target );
void dash( const spell &sp, Creature &caster, const tripoint &target );
void banishment( const spell &sp, Creature &caster, const tripoint &target );
void none( const spell &sp, Creature &, const tripoint &target );

static const std::map<spell_shape, std::function<std::set<tripoint>
( const override_parameters &, const tripoint &, const tripoint & )>> shape_map = {
    { spell_shape::blast, spell_effect_blast },
    { spell_shape::line, spell_effect_line },
    { spell_shape::cone, spell_effect_cone }
};

static const
std::map<std::string, std::function<void( const spell &, Creature &, const tripoint & )>>
effect_map{
    { "pain_split", spell_effect::pain_split },
    { "attack", spell_effect::attack },
    { "targeted_polymorph", spell_effect::targeted_polymorph },
    { "teleport_random", spell_effect::teleport_random },
    { "spawn_item", spell_effect::spawn_ethereal_item },
    { "recover_energy", spell_effect::recover_energy },
    { "summon", spell_effect::spawn_summoned_monster },
    { "summon_vehicle", spell_effect::spawn_summoned_vehicle },
    { "translocate", spell_effect::translocate },
    { "area_pull", spell_effect::area_pull },
    { "area_push", spell_effect::area_push },
    { "directed_push", spell_effect::directed_push },
    { "timed_event", spell_effect::timed_event },
    { "ter_transform", spell_effect::transform_blast },
    { "noise", spell_effect::noise },
    { "vomit", spell_effect::vomit },
    { "explosion", spell_effect::explosion },
    { "flashbang", spell_effect::flashbang },
    { "mod_moves", spell_effect::mod_moves },
    { "map", spell_effect::map },
    { "morale", spell_effect::morale },
    { "charm_monster", spell_effect::charm_monster },
    { "mutate", spell_effect::mutate },
    { "bash", spell_effect::bash },
    { "dash", spell_effect::dash },
    { "banishment", spell_effect::banishment },
    { "none", spell_effect::none }
};
} // namespace spell_effect

class spellbook_callback : public uilist_callback
{
    private:
        std::vector<spell_type> spells;
    public:
        void add_spell( const spell_id &sp );
        void refresh( uilist *menu ) override;
};

// Utility structure to run area queries over weight map. It uses shortest-path-expanding-tree,
// similar to the ones used in pathfinding. Some spell effects, like area_pull use the final
// tree to determine where to move affected objects.
struct area_expander {
    // A single node for a tree.
    struct node {
        // Expanded position
        tripoint position;
        // Previous position
        tripoint from;
        // Accumulated cost.
        float cost = 0.0f;
    };

    int max_range = -1;
    int max_expand = -1;

    // The area we have visited already.
    std::vector<node> area;

    // Maps coordinate to expanded node.
    std::map<tripoint, int> area_search;

    struct area_node_comparator {
        explicit area_node_comparator( std::vector<area_expander::node> &area ) : area( area ) {
        }

        bool operator()( int a, int b ) const {
            return area[a].cost < area[b].cost;
        }

        std::vector<area_expander::node> &area;
    };

    std::priority_queue<int, std::vector<int>, area_node_comparator> frontier;

    area_expander();
    // Check whether we have already visited this node.
    int contains( const tripoint &pt ) const;

    // Adds node to a search tree. Returns true if new node is allocated.
    bool enqueue( const tripoint &from, const tripoint &to, float cost );

    // Run wave propagation
    int run( const tripoint &center );

    // Sort nodes by its cost.
    void sort_ascending();

    void sort_descending();
};

#endif // CATA_SRC_MAGIC_H
