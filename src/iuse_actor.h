#ifndef IUSE_ACTOR_H
#define IUSE_ACTOR_H

#include "iuse.h"
#include "color.h"
#include "field.h"
#include "bodypart.h"

class JsonObject;

/**
 * Transform an item into a specific type.
 * Optionally activate it.
 * Optionally split it in container and content (like opening a jar).
 *
 * It optionally checks for
 * 1. original item has a minimal amount of charges.
 * 2. player has a minimal amount of "fire" charges and consumes them,
 * 3. if fire is used, checks that the player is not underwater.
 */
class iuse_transform : public iuse_actor
{
    public:
        /** Message to the player, %s is replaced with the item name */
        std::string msg_transform;
        /** Id of the resulting item. */
        std::string target_id;
        /**
         * If >= -1: set the charges property of the target to this value. */
        long target_charges;
        /** Id of the container (or empty if no container is needed).
         * If not empty, the item is transformed to the container, and a
         * new item (with type @ref target_id) is placed inside.
         * In that case the new item will have the current turn as birthday.
         */
        std::string container_id;
        /** Set the active property of the resulting item to this. */
        bool active;
        /** Need this many fire charges. Values <= 0 don't need fire.
         * The player must not be underwater if fire is used! */
        long need_fire;
        std::string need_fire_msg;
        /** Need this many charges before processing the action. Values <= 0 are ignored. */
        long need_charges;
        std::string need_charges_msg;
        /** Subtract this from @ref Creature::moves when actually transforming the item. */
        int moves;

        iuse_transform()
            : iuse_actor()
            , target_charges(-2)
            , active(false)
            , need_fire(0)
            , need_charges(0)
            , moves(0)
        {
        }
        virtual ~iuse_transform();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * This is a @ref iuse_transform for active items.
 * It can be called each turn.
 * It does the transformation - either when requested by the user,
 * or when the charges of the item reaches 0.
 * It can display different messages therefor.
 */
class auto_iuse_transform : public iuse_transform
{
    public:
        /**
         * If non-empty: check each turn if the player is underwater
         * and activate the transformation in that case.
         */
        std::string when_underwater;
        /**
         * If non-empty: don't let the user activate the transformation.
         * Instead wait for the item to trigger the transformation
         * (no charges, underwater).
         */
        std::string non_interactive_msg;

        auto_iuse_transform()
            : iuse_transform()
        {
        }
        virtual ~auto_iuse_transform();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * This is a @ref iuse_actor for active items that explode when
 * their charges reaches 0.
 * It can be called each turn, it can make a sound each turn.
 */
class explosion_iuse : public iuse_actor
{
    public:
        // Those 4 values are forwarded to game::explosion.
        // No explosion is done if power < 0
        int explosion_power;
        int explosion_shrapnel;
        bool explosion_fire;
        bool explosion_blast;
        // Those 2 values are forwarded to game::draw_explosion,
        // Nothing is drawn if radius < 0 (game::explosion might still draw something)
        int draw_explosion_radius;
        nc_color draw_explosion_color;
        /** Call game::flashbang? */
        bool do_flashbang;
        bool flashbang_player_immune;
        /** Create fields of this type around the center of the explosion */
        int fields_radius;
        field_id fields_type;
        int fields_min_density;
        int fields_max_density;
        /** Calls game::emp_blast if >= 0 */
        int emp_blast_radius;
        /** Calls game::scrambler_blast if >= 0 */
        int scrambler_blast_radius;
        /** Volume of sound each turn, -1 means no sound at all */
        int sound_volume;
        std::string sound_msg;
        /** Message shown when the player tries to deactivate the item,
         * which is not allowed. */
        std::string no_deactivate_msg;

        explosion_iuse()
            : iuse_actor()
            , explosion_power(-1)
            , explosion_shrapnel(-1)
            , explosion_fire(false)
            , explosion_blast(true) // true is the default in game.h
            , draw_explosion_radius(-1)
            , draw_explosion_color(c_white)
            , do_flashbang(false)
            , flashbang_player_immune(false) // false is the default in game.h
            , fields_radius(-1)
            , fields_type(fd_null)
            , fields_min_density(1)
            , fields_max_density(3)
            , emp_blast_radius(-1)
            , scrambler_blast_radius(-1)
            , sound_volume(-1)
        {
        }
        virtual ~explosion_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * This iuse creates a new vehicle on the map.
 */
class unfold_vehicle_iuse : public iuse_actor
{
    public:
        /** Vehicle name (@see map::add_vehicle what it expects). */
        std::string vehicle_name;
        /** Message shown after successfully unfolding the item. */
        std::string unfold_msg;
        /** Creature::moves it takes to unfold. */
        int moves;
        std::map<std::string, int> tools_needed;
        unfold_vehicle_iuse()
            : iuse_actor()
            , moves(0)
        {
        }
        virtual ~unfold_vehicle_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/** Used in consume_drug_iuse for storing effect data. */
struct effect_data
{
    std::string id;
    int duration;
    body_part bp;
    bool permanent;
    
    effect_data(std::string nid, int dur, body_part nbp, bool perm) :
                    id(nid), duration(dur), bp(nbp), permanent(perm) {};
};

/**
 * This iuse encapsulates the effects of taking a drug.
 */
class consume_drug_iuse : public iuse_actor
{
    public:
        /** Message to display when drug is consumed. **/
        std::string activation_message;
        /** Fields to produce when you take the drug, mostly intended for various kinds of smoke. **/
        std::map<std::string, int> fields_produced;
        /** Tool charges needed to take the drug, e.g. fire. **/
        std::map<std::string, int> charges_needed;
        /** Tools needed, but not consumed, e.g. "smoking apparatus". **/
        std::map<std::string, int> tools_needed;
        /** An effect or effects (conditions) to give the player for the stated duration. **/
        std::vector<effect_data> effects;
        /** A list of stats and adjustments to them. **/
        std::map<std::string, int> stat_adjustments;

        consume_drug_iuse() : iuse_actor() { }
        virtual ~consume_drug_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * This is a @ref iuse_transform for similar to @ref auto_iuse_transform,
 * but it uses the age of the item instead of a counter.
 * The age is calculated from the current turn and the birthday of the item.
 * The player has to activate the item manually, only when the specific
 * age has been reached, it will transform.
 */
class delayed_transform_iuse : public iuse_transform
{
    public:
        /**
         * The minimal age of the item (in turns) to allow the transformation.
         */
        int transform_age;
        /**
         * Message to display when the user activates the item before the
         * age has been reached.
         */
        std::string not_ready_msg;

        /** How much longer (in turns) until the transformation can be done, can be negative. */
        int time_to_do( const item &it ) const;

        delayed_transform_iuse() : iuse_transform(), transform_age(0) { }
        virtual ~delayed_transform_iuse();
        virtual void load( JsonObject &jo );
        virtual long use( player *, item *, bool, point ) const;
        virtual iuse_actor *clone() const;
};

/**
 * This iuse contains the logic to transform a robot item into an actual monster on the map.
 */
class place_monster_iuse : public iuse_actor
{
    public:
        /** The monster type id of the monster to create. */
        std::string mtype_id;
        /** If true, place the monster at a random square around the player,
         * otherwise allow the player to select the target square. */
        bool place_randomly;
        /** How many move points this action takes. */
        int moves;
        /** Difficulty of programming the monster (to be friendly). */
        int difficulty;
        /** Shown when programming the monster succeeded and it's friendly. Can be empty. */
        std::string friendly_msg;
        /** Shown when programming the monster failed and it's hostile. Can be empty. */
        std::string hostile_msg;
        /** Skills used to make the monster not hostile when activated. **/
        std::string skill1;
        std::string skill2;

        place_monster_iuse() : iuse_actor(), place_randomly( false ), moves( 100 ), difficulty( 0 ) { }
        virtual ~place_monster_iuse();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * Items that can be worn and can be activated to consume energy from UPS.
 * Note that the energy consumption is done in @ref player::process_active_items, it is
 * *not* done by this class!
 */
class ups_based_armor_actor : public iuse_actor
{
    public:
        /** Shown when activated. */
        std::string activate_msg;
        /** Shown when deactivated. */
        std::string deactive_msg;
        /** Shown when it runs out of power. */
        std::string out_of_power_msg;

        ups_based_armor_actor() : iuse_actor() { }
        virtual ~ups_based_armor_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * This implements lock picking.
 */
class pick_lock_actor : public iuse_actor
{
    public:
        /**
         * How good the used tool is at picking a lock.
         */
        int pick_quality;

        pick_lock_actor() : iuse_actor(), pick_quality( 0 ) { }
        virtual ~pick_lock_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

/**
 * Reveals specific things on the overmap.
 */
class reveal_map_actor : public iuse_actor
{
    public:
        /**
         * The radius of the overmap area that gets revealed.
         * This is in overmap terrain coordinates. A radius of 1 means all terrains directly around
         * the character are revealed.
         */
        int radius;
        /**
         * Overmap terrain types that get revealed.
         */
        std::vector<std::string> omt_types;
        /**
         * The message displayed after revealing.
         */
        std::string message;

        void reveal_targets( const std::string &target, int reveal_distance ) const;

        reveal_map_actor() : iuse_actor(), radius( 0 ) { }
        virtual ~reveal_map_actor();
        virtual void load( JsonObject &jo );
        virtual long use(player *, item *, bool, point) const;
        virtual iuse_actor *clone() const;
};

#endif
