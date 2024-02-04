#pragma once
#ifndef CATA_SRC_FLAG_H
#define CATA_SRC_FLAG_H

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename T> class generic_factory;

extern const flag_id flag_NULL;
extern const flag_id flag_ABLATIVE_LARGE;
extern const flag_id flag_ABLATIVE_MEDIUM;
extern const flag_id flag_ACID;
extern const flag_id flag_ACID_IMMUNE;
extern const flag_id flag_ACTIVATE_ON_PLACE;
extern const flag_id flag_ACTIVE_CLOAKING;
extern const flag_id flag_ACTIVE_GENERATOR;
extern const flag_id flag_ACT_IN_FIRE;
extern const flag_id flag_ACT_ON_RANGED_HIT;
extern const flag_id flag_ALARMCLOCK;
extern const flag_id flag_ALLERGEN_EGG;
extern const flag_id flag_ALLERGEN_FRUIT;
extern const flag_id flag_ALLERGEN_JUNK;
extern const flag_id flag_ALLERGEN_MEAT;
extern const flag_id flag_ALLERGEN_MILK;
extern const flag_id flag_ALLERGEN_NUT;
extern const flag_id flag_ALLERGEN_VEGGY;
extern const flag_id flag_ALLERGEN_DRIED_VEGETABLE;
extern const flag_id flag_ALLERGEN_WHEAT;
extern const flag_id flag_ALLERGEN_BREAD;
extern const flag_id flag_ALLERGEN_WOOL;
extern const flag_id flag_ALLERGEN_CHEESE;
extern const flag_id flag_ALLOWS_BODY_BLOCK;
extern const flag_id flag_ALLOWS_NATURAL_ATTACKS;
extern const flag_id flag_ALLOWS_REMOTE_USE;
extern const flag_id flag_ALWAYS_TWOHAND;
extern const flag_id flag_ANIMAL_PRODUCT;
extern const flag_id flag_OLD_CURRENCY;
extern const flag_id flag_AURA;
extern const flag_id flag_BAROMETER;
extern const flag_id flag_BASH_IMMUNE;
extern const flag_id flag_BELTED;
extern const flag_id flag_BELT_CLIP;
extern const flag_id flag_BIO_IMMUNE;
extern const flag_id flag_BIONIC_FUEL_SOURCE;
extern const flag_id flag_BIONIC_GUN;
extern const flag_id flag_BIONIC_INSTALLATION_DATA;
extern const flag_id flag_BIONIC_TOGGLED;
extern const flag_id flag_BIONIC_WEAPON;
extern const flag_id flag_BIPOD;
extern const flag_id flag_BIRD;
extern const flag_id flag_BLED;
extern const flag_id flag_BLIND;
extern const flag_id flag_BLOCK_WHILE_WORN;
extern const flag_id flag_BOMB;
extern const flag_id flag_BRASS_CATCHER;
extern const flag_id flag_BULLET_IMMUNE;
extern const flag_id flag_BURNOUT;
extern const flag_id flag_BYPRODUCT;
extern const flag_id flag_CABLE_SPOOL;
extern const flag_id flag_CAMERA_PRO;
extern const flag_id flag_CANNIBAL;
extern const flag_id flag_CANNIBALISM;
extern const flag_id flag_CANT_HEAL_EVERYONE;
extern const flag_id flag_CANT_WEAR;
extern const flag_id flag_CARNIVORE_OK;
extern const flag_id flag_CASING;
extern const flag_id flag_CATTLE;
extern const flag_id flag_CHALLENGE;
extern const flag_id flag_CHARGEDIM;
extern const flag_id flag_CHOKE;
extern const flag_id flag_CITY_START;
extern const flag_id flag_COLD;
extern const flag_id flag_COLD_IMMUNE;
extern const flag_id flag_COLLAPSED_STOCK;
extern const flag_id flag_COLLAPSE_CONTENTS;
extern const flag_id flag_COLLAPSIBLE_STOCK;
extern const flag_id flag_COLLAR;
extern const flag_id flag_COMBAT_TOGGLEABLE;
extern const flag_id flag_CONDUCTIVE;
extern const flag_id flag_CONSUMABLE;
extern const flag_id flag_COOKED;
extern const flag_id flag_CORROSIVE;
extern const flag_id flag_CORPSE;
extern const flag_id flag_CRUTCHES;
extern const flag_id flag_CUSTOM_EXPLOSION;
extern const flag_id flag_CUT_HARVEST;
extern const flag_id flag_CUT_IMMUNE;
extern const flag_id flag_DANGEROUS;
extern const flag_id flag_DEAF;
extern const flag_id flag_DIAMOND;
extern const flag_id flag_DIG_TOOL;
extern const flag_id flag_DIMENSIONAL_ANCHOR;
extern const flag_id flag_DIRTY;
extern const flag_id flag_DISABLE_SIGHTS;
extern const flag_id flag_DROP_ACTION_ONLY_IF_LIQUID;
extern const flag_id flag_DURABLE_MELEE;
extern const flag_id flag_EATEN_COLD;
extern const flag_id flag_EATEN_HOT;
extern const flag_id flag_EDIBLE_FROZEN;
extern const flag_id flag_EFFECT_IMPEDING;
extern const flag_id flag_EFFECT_LIMB_SCORE_MOD;
extern const flag_id flag_EFFECT_LIMB_SCORE_MOD_LOCAL;
extern const flag_id flag_ELECTRIC_IMMUNE;
extern const flag_id flag_ELECTRONIC;
extern const flag_id flag_ETHEREAL_ITEM;
extern const flag_id flag_EXO_ARM_PLATE;
extern const flag_id flag_EXO_BOOT_PLATE;
extern const flag_id flag_EXO_GLOVE_PLATE;
extern const flag_id flag_EXO_HELMET_GADGET;
extern const flag_id flag_EXO_HELMET_PLATE;
extern const flag_id flag_EXO_LARGE_GADGET;
extern const flag_id flag_EXO_LEG_PLATE;
extern const flag_id flag_EXO_MEDIUM_GADGET;
extern const flag_id flag_EXO_PSU;
extern const flag_id flag_EXO_PSU_PLATE;
extern const flag_id flag_EXO_SMALL_GADGET;
extern const flag_id flag_EXO_TORSO_PLATE;
extern const flag_id flag_EXO_UNDERLAYER;
extern const flag_id flag_FAKE_MILL;
extern const flag_id flag_FAKE_SMOKE;
extern const flag_id flag_FANCY;
extern const flag_id flag_FELINE;
extern const flag_id flag_FERTILIZER;
extern const flag_id flag_FIELD_DRESS;
extern const flag_id flag_FIELD_DRESS_FAILED;
extern const flag_id flag_FILTHY;
extern const flag_id flag_FIN;
extern const flag_id flag_FIRE;
extern const flag_id flag_FIRESTARTER;
extern const flag_id flag_FIREWOOD;
extern const flag_id flag_FIRE_100;
extern const flag_id flag_FIRE_20;
extern const flag_id flag_FIRE_50;
extern const flag_id flag_FIRE_TWOHAND;
extern const flag_id flag_FIT;
extern const flag_id flag_FIX_FARSIGHT;
extern const flag_id flag_FIX_NEARSIGHT;
extern const flag_id flag_FLAMING;
extern const flag_id flag_FLASH_PROTECTION;
extern const flag_id flag_FLOTATION;
extern const flag_id flag_FOLDED_STOCK;
extern const flag_id flag_FORAGE_HALLU;
extern const flag_id flag_FORAGE_POISON;
extern const flag_id flag_FRAGILE;
extern const flag_id flag_FRAGILE_MELEE;
extern const flag_id flag_FRESH_GRAIN;
extern const flag_id flag_FREEZERBURN;
extern const flag_id flag_FROZEN;
extern const flag_id flag_FUNGAL_VECTOR;
extern const flag_id flag_GAS_DISCOUNT;
extern const flag_id flag_GAS_PROOF;
extern const flag_id flag_GIBBED;
extern const flag_id flag_GNV_EFFECT;
extern const flag_id flag_HARVEST_SEEDS;
extern const flag_id flag_HEAT_IMMUNE;
extern const flag_id flag_HIDDEN_HALLU;
extern const flag_id json_flag_HIDDEN_ITEM;
extern const flag_id flag_HIDDEN_POISON;
extern const flag_id flag_HOOD;
extern const flag_id flag_HOT;
extern const flag_id flag_HURT_WHEN_WIELDED;
extern const flag_id flag_HYGROMETER;
extern const flag_id flag_INEDIBLE;
extern const flag_id flag_INITIAL_PART;
extern const flag_id flag_INSPIRATIONAL;
extern const flag_id flag_INSTALL_DIFFICULT;
extern const flag_id flag_IN_CBM;
extern const flag_id flag_INTEGRATED;
extern const flag_id flag_IRRADIATED;
extern const flag_id flag_IRREMOVABLE;
extern const flag_id flag_IR_EFFECT;
extern const flag_id flag_IS_ARMOR;
extern const flag_id flag_IS_PET_ARMOR;
extern const flag_id flag_IS_UPS;
extern const flag_id flag_ITEM_BROKEN;
extern const flag_id flag_LASER_SIGHT;
extern const flag_id flag_LEAK_ALWAYS;
extern const flag_id flag_LEAK_DAM;
extern const flag_id flag_LITCIG;
extern const flag_id flag_LUPINE;
extern const flag_id flag_MAGICAL;
extern const flag_id flag_MAGIC_FOCUS;
extern const flag_id flag_MAG_BELT;
extern const flag_id flag_MAG_BULKY;
extern const flag_id flag_MAG_DESTROY;
extern const flag_id flag_MAG_EJECT;
extern const flag_id flag_MC_HAS_DATA;
extern const flag_id flag_MC_MOBILE;
extern const flag_id flag_MECH_BAT;
extern const flag_id flag_MELTS;
extern const flag_id flag_MESSY;
extern const flag_id flag_MISSION_ITEM;
extern const flag_id flag_MOUNTED_GUN;
extern const flag_id flag_MOUSE;
extern const flag_id flag_MUNDANE;
extern const flag_id flag_MUSHY;
extern const flag_id flag_MYCUS_OK;
extern const flag_id flag_NANOFAB_REPAIR;
extern const flag_id flag_NANOFAB_TEMPLATE;
extern const flag_id flag_NANOFAB_TEMPLATE_SINGLE_USE;
extern const flag_id flag_NEEDS_NO_LUBE;
extern const flag_id flag_NEEDS_UNFOLD;
extern const flag_id flag_NEGATIVE_MONOTONY_OK;
extern const flag_id flag_NEVER_JAMS;
extern const flag_id flag_NONCONDUCTIVE;
extern const flag_id flag_NON_FOULING;
extern const flag_id flag_NORMAL;
extern const flag_id flag_NO_CVD;
extern const flag_id flag_NO_DROP;
extern const flag_id flag_NO_INGEST;
extern const flag_id flag_NO_PACKED;
extern const flag_id flag_NO_PARASITES;
extern const flag_id flag_NO_RELOAD;
extern const flag_id flag_NO_REPAIR;
extern const flag_id flag_NO_SALVAGE;
extern const flag_id flag_NO_STERILE;
extern const flag_id flag_NO_TAKEOFF;
extern const flag_id flag_NO_TURRET;
extern const flag_id flag_NO_UNLOAD;
extern const flag_id flag_NO_UNWIELD;
extern const flag_id flag_NO_WEAR_EFFECT;
extern const flag_id flag_NPC_ACTIVATE;
extern const flag_id flag_NPC_ALT_ATTACK;
extern const flag_id flag_NPC_SAFE;
extern const flag_id flag_NPC_THROWN;
extern const flag_id flag_NPC_THROW_NOW;
extern const flag_id flag_NUTRIENT_OVERRIDE;
extern const flag_id flag_ONLY_ONE;
extern const flag_id flag_ORGANIC;
extern const flag_id flag_OUTER;
extern const flag_id flag_OVERSIZE;
extern const flag_id flag_PADDED;
extern const flag_id flag_PAIN_IMMUNE;
extern const flag_id flag_PALS_SMALL;
extern const flag_id flag_PALS_MEDIUM;
extern const flag_id flag_PALS_LARGE;
extern const flag_id flag_PARTIAL_DEAF;
extern const flag_id flag_PERFECT_LOCKPICK;
extern const flag_id flag_PERPETUAL;
extern const flag_id flag_PERSONAL;
extern const flag_id flag_PLACE_RANDOMLY;
extern const flag_id flag_POCKETS;
extern const flag_id flag_POLEARM;
extern const flag_id flag_POWERARMOR_COMPATIBLE;
extern const flag_id flag_POWERED;
extern const flag_id flag_PREDATOR_FUN;
extern const flag_id flag_PRIMITIVE_RANGED_WEAPON;
extern const flag_id flag_PROCESSING;
extern const flag_id flag_PROCESSING_RESULT;
extern const flag_id flag_PSEUDO;
extern const flag_id flag_PSYSHIELD_PARTIAL;
extern const flag_id flag_PULPED;
extern const flag_id flag_PUMP_ACTION;
extern const flag_id flag_PUMP_RAIL_COMPATIBLE;
extern const flag_id flag_QUARTERED;
extern const flag_id flag_RABBIT;
extern const flag_id flag_RADIOACTIVE;
extern const flag_id flag_RADIOCAR;
extern const flag_id flag_RADIOCARITEM;
extern const flag_id flag_RADIOSIGNAL_1;
extern const flag_id flag_RADIOSIGNAL_2;
extern const flag_id flag_RADIOSIGNAL_3;
extern const flag_id flag_RADIO_ACTIVATION;
extern const flag_id flag_RADIO_CONTAINER;
extern const flag_id flag_RADIO_INVOKE_PROC;
extern const flag_id flag_RADIO_MOD;
extern const flag_id flag_RADIO_MODABLE;
extern const flag_id flag_RAD_PROOF;
extern const flag_id flag_RAD_RESIST;
extern const flag_id flag_RAINPROOF;
extern const flag_id flag_RAIN_PROTECT;
extern const flag_id flag_RAT;
extern const flag_id flag_RAW;
extern const flag_id flag_REACH3;
extern const flag_id flag_REACH;
extern const flag_id flag_REACH_ATTACK;
extern const flag_id flag_REBREATHER;
extern const flag_id flag_RECHARGE;
extern const flag_id flag_REDUCED_BASHING;
extern const flag_id flag_REDUCED_WEIGHT;
extern const flag_id flag_RELOAD_AND_SHOOT;
extern const flag_id flag_RELOAD_EJECT;
extern const flag_id flag_RELOAD_ONE;
extern const flag_id flag_REMOVED_STOCK;
extern const flag_id flag_REQUIRES_BALANCE;
extern const flag_id flag_REQUIRES_TINDER;
extern const flag_id flag_RESTRICT_HANDS;
extern const flag_id flag_REVIVE_SPECIAL;
extern const flag_id flag_ROLLER_INLINE;
extern const flag_id flag_ROLLER_ONE;
extern const flag_id flag_ROLLER_QUAD;
extern const flag_id flag_SAFECRACK;
extern const flag_id flag_SEMITANGIBLE;
extern const flag_id flag_SHRUB;
extern const flag_id flag_SINGLE_USE;
extern const flag_id flag_SKINNED;
extern const flag_id flag_SKINTIGHT;
extern const flag_id flag_SLEEP_AID;
extern const flag_id flag_SLEEP_AID_CONTAINER;
extern const flag_id flag_SLEEP_IGNORE;
extern const flag_id flag_SLOWS_MOVEMENT;
extern const flag_id flag_SLOWS_THIRST;
extern const flag_id flag_SLOW_WIELD;
extern const flag_id flag_SMOKABLE;
extern const flag_id flag_SMOKED;
extern const flag_id flag_SOLARPACK;
extern const flag_id flag_SOLARPACK_ON;
extern const flag_id flag_SEED_HARVEST;
extern const flag_id flag_SPAWN_ACTIVE;
extern const flag_id flag_SPEAR;
extern const flag_id flag_SPEEDLOADER;
extern const flag_id flag_SPEEDLOADER_CLIP;
extern const flag_id flag_SPLINT;
extern const flag_id flag_STAB;
extern const flag_id flag_STAB_IMMUNE;
extern const flag_id flag_STRICT_HUMANITARIANISM;
extern const flag_id flag_STR_DRAW;
extern const flag_id flag_STR_RELOAD;
extern const flag_id flag_STURDY;
extern const flag_id flag_SUN_GLASSES;
extern const flag_id flag_SUPER_FANCY;
extern const flag_id flag_SWIM_GOGGLES;
extern const flag_id flag_TACK;
extern const flag_id flag_TANGLE;
extern const flag_id flag_TARDIS;
extern const flag_id flag_THERMOMETER;
extern const flag_id flag_THIN_OBSTACLE;
extern const flag_id flag_TIE_UP;
extern const flag_id flag_TINDER;
extern const flag_id flag_TOBACCO;
extern const flag_id flag_TOUGH_FEET;
extern const flag_id flag_TOURNIQUET;
extern const flag_id flag_TOW_CABLE;
extern const flag_id flag_TRADER_AVOID;
extern const flag_id flag_TRADER_KEEP;
extern const flag_id flag_TRADER_KEEP_EQUIPPED;
extern const flag_id flag_TRANSPARENT;
extern const flag_id flag_TWO_WAY_RADIO;
extern const flag_id flag_UNBREAKABLE;
extern const flag_id flag_UNBREAKABLE_MELEE;
extern const flag_id flag_UNDERFED;
extern const flag_id flag_UNDERSIZE;
extern const flag_id flag_UNDERWATER_GUN;
extern const flag_id flag_UNRECOVERABLE;
extern const flag_id flag_UNRESTRICTED;
extern const flag_id flag_URSINE_HONEY;
extern const flag_id flag_USES_BIONIC_POWER;
extern const flag_id flag_USE_EAT_VERB;
extern const flag_id flag_USE_PLAYER_ENERGY;
extern const flag_id flag_USE_POWER_WHEN_HIT;
extern const flag_id flag_USE_UPS;
extern const flag_id flag_VARSIZE;
extern const flag_id flag_VEHICLE;
extern const flag_id flag_WAIST;
extern const flag_id flag_WATCH;
extern const flag_id flag_WATERPROOF;
extern const flag_id flag_WATERPROOF_GUN;
extern const flag_id flag_WATER_BREAK;
extern const flag_id flag_WATER_BREAK_ACTIVE;
extern const flag_id flag_WATER_EXTINGUISH;
extern const flag_id flag_WATER_FRIENDLY;
extern const flag_id flag_WATER_DISSOLVE;
extern const flag_id flag_WET;
extern const flag_id flag_WHIP;
extern const flag_id flag_WIND_EXTINGUISH;
extern const flag_id flag_WRITE_MESSAGE;
extern const flag_id flag_ZERO_WEIGHT;
extern const flag_id flag_ZOOM;
extern const flag_id flag_wooled;
extern const flag_id flag_WONT_TRAIN_MARKSMANSHIP;
extern const flag_id flag_MUTE;
extern const flag_id flag_NO_CLEAN;
extern const flag_id flag_SOFT;
extern const flag_id flag_HARD;
extern const flag_id flag_SHREDDED;
extern const flag_id flag_FROM_FROZEN_LIQUID;
/**
 * Flags: json entity with "type": "json_flag", defined in flags.json, vp_flags.json
 * Currently used by:
 *   * Items (itype, item), as int flag_id
 *   * Vehicles (vehicle_part), only to display flag info
 *   * Effects (effect_type), as int flag_id
 *   * Recipes (recipe), flag_id in recipe::flags_to_delete
 *
 * When flag is used in any of these places, it must have it's definition in json
 * (otherwise flag_id -> flag_id conversion can't be performed).
 */
class json_flag
{
        friend class DynamicDataLoader;
        friend class generic_factory<json_flag>;

    public:
        // used by generic_factory
        flag_id id = flag_NULL;
        std::vector<std::pair<flag_id, mod_id>> src;
        bool was_loaded = false;

        json_flag() = default;

        /** Fetches flag definition (or null flag if not found) */
        static const json_flag &get( const std::string &id );

        /** Get informative text for display in UI */
        std::string info() const {
            return info_.translated();
        }

        /** Get "restriction" phrase, saying what items with this flag must be able to do */
        std::string restriction() const {
            return restriction_.translated();
        }

        /** Get name of the flag. */
        std::string name() const {
            return name_.translated();
        }

        /** Is flag inherited by base items from any attached items? */
        bool inherit() const {
            return inherit_;
        }

        /** Is flag inherited by crafted items from any component items? */
        bool craft_inherit() const {
            return craft_inherit_;
        }

        /** Requires this flag to be installed on vehicle */
        const std::string &requires_flag() const {
            return requires_flag_;
        }

        /** The flag's modifier on the fun value of comestibles */
        int taste_mod() const {
            return taste_mod_;
        }

        /** Is this a valid (non-null) flag */
        explicit operator bool() const;

        void check() const;

        /** true, if flags were loaded */
        static bool is_ready();

        static const std::vector<json_flag> &get_all();

    private:
        translation info_;
        translation restriction_;
        translation name_;
        std::set<std::string> conflicts_;
        bool inherit_ = true;
        bool craft_inherit_ = false;
        std::string requires_flag_;
        int taste_mod_ = 0;

        /** Load flag definition from JSON */
        void load( const JsonObject &jo, std::string_view src );

        /** Load all flags from JSON */
        static void load_all( const JsonObject &jo, const std::string &src );

        /** finalize */
        static void finalize_all( );

        /** Check consistency of all loaded flags */
        static void check_consistency();

        /** Clear all loaded flags (invalidating any pointers) */
        static void reset();
};

#endif // CATA_SRC_FLAG_H
