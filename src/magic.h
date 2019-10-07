#pragma once
#ifndef MAGIC_H
#define MAGIC_H

#include <stddef.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <queue>

#include "bodypart.h"
#include "damage.h"
#include "enum_bitset.h"
#include "type_id.h"
#include "ui.h"
#include "string_id.h"
#include "translations.h"
#include "event_bus.h"
#include "sounds.h"

struct tripoint;
class Creature;
class player;
class spell;
class JsonObject;
class JsonOut;
class JsonIn;
class spell;
class teleporter_list;
class time_duration;
class nc_color;
template <typename E> struct enum_traits;

enum spell_flag {
    PERMANENT, // items or creatures spawned with this spell do not disappear and die as normal
    IGNORE_WALLS, // spell's aoe goes through walls
    HOSTILE_SUMMON, // summon spell always spawns a hostile monster
    HOSTILE_50, // summoned monster spawns friendly 50% of the time
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
    MUTATE_TRAIT, // overrides the mutate spell_effect to use a specific trait_id instead of a category
    WONDER, // instead of casting each of the extra_spells, it picks N of them and casts them (where N is std::min( damage(), number_of_spells ))
    LAST
};

enum energy_type {
    hp_energy,
    mana_energy,
    stamina_energy,
    bionic_energy,
    fatigue_energy,
    none_energy
};

enum valid_target {
    target_ally,
    target_hostile,
    target_self,
    target_ground,
    target_none,
    target_item,
    target_fd_fire,
    target_fd_blood,
    _LAST
};

template<>
struct enum_traits<valid_target> {
    static constexpr auto last = valid_target::_LAST;
};

template<>
struct enum_traits<spell_flag> {
    static constexpr auto last = spell_flag::LAST;
};

struct fake_spell {
    spell_id id;
    // max level this spell can be
    // if null pointer, spell can be up to its own max level
    cata::optional<int> max_level;
    // level for things that need it
    int level;
    // target tripoint is source (true) or target (false)
    bool self;

    fake_spell() = default;
    fake_spell( const spell_id &sp_id, bool hit_self = false,
                const cata::optional<int> &max_level = cata::nullopt ) : id( sp_id ),
        max_level( max_level ), self( hit_self ) {}

    spell get_spell( int input_level ) const;

    void load( JsonObject &jo );
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
        sounds::sound_t sound_type;
        bool sound_ambient;
        std::string sound_id;
        std::string sound_variant;
        // spell effect string. used to look up spell function
        std::string effect_name;
        std::function<void( const spell &, Creature &, const tripoint & )> effect;
        // extra information about spell effect. allows for combinations for effects
        std::string effect_str;
        // list of additional "spell effects"
        std::vector<fake_spell> additional_spells;

        // if the spell has a field name defined, this is where it is
        cata::optional<field_type_id> field;
        // the chance one_in( field_chance ) that the field spawns at a tripoint in the area of the spell
        int field_chance;
        // field intensity at spell level 0
        int min_field_intensity;
        // increment of field intensity per level
        float field_intensity_increment;
        // maximum field intensity allowed
        int max_field_intensity;
        // field intensity added to the map is +- ( 1 + field_intensity_variance ) * field_intensity
        float field_intensity_variance;

        // minimum damage this spell can cause
        int min_damage;
        // amount of damage increase per spell level
        float damage_increment;
        // maximum damage this spell can cause
        int max_damage;

        // minimum range of a spell
        int min_range;
        // amount of range increase per spell level
        float range_increment;
        // max range this spell can achieve
        int max_range;

        // minimum area of effect of a spell (radius)
        // 0 means the spell only affects the target
        int min_aoe;
        // amount of area of effect increase per spell level (radius)
        float aoe_increment;
        // max area of effect of a spell (radius)
        int max_aoe;

        // damage over time deals damage per turn

        // minimum damage over time
        int min_dot;
        // increment per spell level
        float dot_increment;
        // max damage over time
        int max_dot;

        // amount of time effect lasts

        // minimum time for effect in moves
        int min_duration;
        // increment per spell level in moves
        // DoT is per turn, but increments can be smaller
        int duration_increment;
        // max time for effect in moves
        int max_duration;

        // amount of damage that is piercing damage
        // not added to damage stat

        // minimum pierce damage
        int min_pierce;
        // increment of pierce damage per spell level
        float pierce_increment;
        // max pierce damage
        int max_pierce;

        // base energy cost of spell
        int base_energy_cost;
        // increment of energy cost per spell level
        float energy_increment;
        // max or min energy cost, based on sign of energy_increment
        int final_energy_cost;

        // spell is restricted to being cast by only this class
        // if spell_class is empty, spell is unrestricted
        trait_id spell_class;

        // the difficulty of casting a spell
        int difficulty;

        // max level this spell can achieve
        int max_level;

        // base amount of time to cast the spell in moves
        int base_casting_time;
        // increment of casting time per level
        float casting_time_increment;
        // max or min casting time
        int final_casting_time;

        // Does leveling this spell lead to learning another spell?
        std::map<std::string, int> learn_spells;

        // what energy do you use to cast this spell
        energy_type energy_source;

        damage_type dmg_type;

        // list of valid targets to be affected by the area of effect.
        enum_bitset<valid_target> effect_targets;

        // list of valid targets enum
        enum_bitset<valid_target> valid_targets;

        // lits of bodyparts this spell applies its effect to
        enum_bitset<body_part> affected_bps;

        enum_bitset<spell_flag> spell_tags;

        static void load_spell( JsonObject &jo, const std::string &src );
        void load( JsonObject &jo, const std::string & );
        /**
         * All spells in the game.
         */
        static const std::vector<spell_type> &get_all();
        static void check_consistency();
        static void reset_all();
        bool is_valid() const;
};

class spell
{
    private:
        friend class spell_events;
        // basic spell data
        spell_id type;

        // once you accumulate enough exp you level the spell
        int experience;
        // returns damage type for the spell
        damage_type dmg_type() const;

        // alternative cast message
        translation alt_message;

        // minimum damage including levels
        int min_leveled_damage() const;
        // minimum aoe including levels
        int min_leveled_aoe() const;
        // minimum duration including levels (moves)
        int min_leveled_duration() const;

    public:
        spell() = default;
        spell( spell_id sp, int xp = 0 );
        spell( spell_id sp, translation alt_msg );

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
        int casting_exp( const player &p ) const;
        // modifier for gaining exp
        float exp_modifier( const player &p ) const;
        // level up!
        void gain_level();
        // is the spell at max level?
        bool is_max_level() const;
        // what is the max level of the spell
        int get_max_level() const;

        // what is the intensity of the field the spell generates ( 0 if no field )
        int field_intensity() const;
        // how much damage does the spell do
        int damage() const;
        dealt_damage_instance get_dealt_damage_instance() const;
        damage_instance get_damage_instance() const;
        // how big is the spell's radius
        int aoe() const;
        // distance spell can be cast
        int range() const;
        // how much energy does the spell cost
        int energy_cost( const player &p ) const;
        // how long does this spell's effect last
        int duration() const;
        time_duration duration_turns() const;
        // how often does the spell fail
        // based on difficulty, level of spell, spellcraft skill, intelligence
        float spell_fail( const player &p ) const;
        std::string colorized_fail_percent( const player &p ) const;
        // how long does it take to cast the spell
        int casting_time( const player &p ) const;

        // can the player cast this spell?
        bool can_cast( const player &p ) const;
        // can the player learn this spell?
        bool can_learn( const player &p ) const;
        // is this spell valid
        bool is_valid() const;
        // is the bodypart affected by the effect
        bool bp_is_affected( body_part bp ) const;
        // check if the spell has a particular flag
        bool has_flag( const spell_flag &flag ) const;
        // check if the spell's class is the same as input
        bool is_spell_class( const trait_id &mid ) const;

        bool in_aoe( const tripoint &source, const tripoint &target ) const;

        // get spell id (from type)
        spell_id id() const;
        // get spell class (from type)
        trait_id spell_class() const;
        // get spell effect string (from type)
        std::string effect() const;
        // get spell effect_str data
        std::string effect_data() const;
        // name of spell (translated)
        std::string name() const;
        // description of spell (translated)
        std::string description() const;
        // spell message when cast (translated)
        std::string message() const;
        // energy source as a string (translated)
        std::string energy_string() const;
        // energy cost returned as a string
        std::string energy_cost_string( const player &p ) const;
        // current energy the player has available as a string
        std::string energy_cur_string( const player &p ) const;
        // prints out a list of valid targets separated by commas
        std::string enumerate_targets() const;

        std::string damage_string() const;
        std::string aoe_string() const;
        std::string duration_string() const;

        // energy source enum
        energy_type energy_source() const;
        // the color that's representative of the damage type
        nc_color damage_type_color() const;
        std::string damage_type_string() const;
        // your level in this spell
        int get_level() const;
        // difficulty of the level
        int get_difficulty() const;

        // tries to create a field at the location specified
        void create_field( const tripoint &at ) const;

        // makes a spell sound at the location
        void make_sound( const tripoint &target ) const;
        void make_sound( const tripoint &target, int loudness ) const;
        // heals the critter at the location, returns amount healed (player heals each body part)
        int heal( const tripoint &target ) const;

        // casts the spell effect. returns true if successful
        void cast_spell_effect( Creature &source, const tripoint &target ) const;
        // goes through the spell effect and all of its internal spells
        void cast_all_effects( Creature &source, const tripoint &target ) const;

        // is the target valid for this spell?
        bool is_valid_target( const Creature &caster, const tripoint &p ) const;
        bool is_valid_target( valid_target t ) const;
        bool is_valid_effect_target( valid_target t ) const;

        // picks a random valid tripoint from @area
        tripoint random_valid_target( const Creature &caster, const tripoint &caster_pos ) const;
};

class known_magic
{
    private:
        // list of spells known
        std::map<spell_id, spell> spellbook;
        // invlets assigned to spell_id
        std::map<spell_id, int> invlets;
        // the base mana a player would start with
        int mana_base;
        // current mana
        int mana;
    public:
        // ignores all distractions when casting a spell when true
        bool casting_ignore = false;

        known_magic();

        void learn_spell( const std::string &sp, player &p, bool force = false );
        void learn_spell( const spell_id &sp, player &p, bool force = false );
        void learn_spell( const spell_type *sp, player &p, bool force = false );
        void forget_spell( const std::string &sp );
        void forget_spell( const spell_id &sp );
        // time in moves for the player to memorize the spell
        int time_to_learn_spell( const player &p, const spell_id &sp ) const;
        int time_to_learn_spell( const player &p, const std::string &str ) const;
        bool can_learn_spell( const player &p, const spell_id &sp ) const;
        bool knows_spell( const std::string &sp ) const;
        bool knows_spell( const spell_id &sp ) const;
        // does the player know a spell?
        bool knows_spell() const;
        // spells known by player
        std::vector<spell_id> spells() const;
        // gets the spell associated with the spell_id to be edited
        spell &get_spell( const spell_id &sp );
        // opens up a ui that the player can choose a spell from
        // returns the index of the spell in the vector of spells
        int select_spell( const player &p );
        // get all known spells
        std::vector<spell *> get_spells();
        // how much mana is available to use to cast spells
        int available_mana() const;
        // max mana vailable
        int max_mana( const player &p ) const;
        void mod_mana( const player &p, int add_mana );
        void set_mana( int new_mana );
        void update_mana( const player &p, float turns );
        // does the player have enough energy to cast this spell?
        // not specific to mana
        bool has_enough_energy( const player &p, spell &sp ) const;

        void on_mutation_gain( const trait_id &mid, player &p );
        void on_mutation_loss( const trait_id &mid );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
    private:
        // gets length of longest spell name
        size_t get_spellname_max_width();
        // gets invlet if assigned, or -1 if not
        int get_invlet( const spell_id &sp, std::set<int> &used_invlets );
};

namespace spell_effect
{

void teleport_random( const spell &sp, Creature &caster, const tripoint & );
void pain_split( const spell &, Creature &, const tripoint & );
void target_attack( const spell &sp, Creature &caster,
                    const tripoint &epicenter );
void projectile_attack( const spell &sp, Creature &caster,
                        const tripoint &target );
void cone_attack( const spell &sp, Creature &caster,
                  const tripoint &target );
void line_attack( const spell &sp, Creature &caster,
                  const tripoint &target );

void area_pull( const spell &sp, Creature &caster, const tripoint &center );
void area_push( const spell &sp, Creature &caster, const tripoint &center );

std::set<tripoint> spell_effect_blast( const spell &, const tripoint &, const tripoint &target,
                                       int aoe_radius, bool ignore_walls );
std::set<tripoint> spell_effect_cone( const spell &sp, const tripoint &source,
                                      const tripoint &target,
                                      int aoe_radius, bool ignore_walls );
std::set<tripoint> spell_effect_line( const spell &, const tripoint &source,
                                      const tripoint &target,
                                      int aoe_radius, bool ignore_walls );

void spawn_ethereal_item( const spell &sp, Creature &, const tripoint & );
void recover_energy( const spell &sp, Creature &, const tripoint &target );
void spawn_summoned_monster( const spell &sp, Creature &caster, const tripoint &target );
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
void mutate( const spell &sp, Creature &caster, const tripoint &target );
void none( const spell &sp, Creature &, const tripoint &target );
} // namespace spell_effect

class spellbook_callback : public uilist_callback
{
    private:
        std::vector<spell_type> spells;
    public:
        void add_spell( const spell_id &sp );
        void select( int entnum, uilist *menu ) override;
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
        float cost = 0;
    };

    int max_range = -1;
    int max_expand = -1;

    // The area we have visited already.
    std::vector<node> area;

    // Maps coordinate to expanded node.
    std::map<tripoint, int> area_search;

    struct area_node_comparator {
        area_node_comparator( std::vector<area_expander::node> &area ) : area( area ) {
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

#endif
