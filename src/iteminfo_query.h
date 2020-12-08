#pragma once
#ifndef CATA_SRC_ITEMINFO_QUERY_H
#define CATA_SRC_ITEMINFO_QUERY_H

#include <cstddef>
#include <bitset>
#include <string>
#include <vector>

enum class iteminfo_parts : size_t {
    BASE_CATEGORY = 0,
    BASE_PRICE,
    BASE_BARTER,
    BASE_OWNER,
    BASE_VOLUME,
    BASE_WEIGHT,
    BASE_LENGTH,
    BASE_RIGIDITY,
    BASE_DAMAGE,
    BASE_TOHIT,
    BASE_MOVES,
    BASE_DPS,
    BASE_REQUIREMENTS,
    BASE_MATERIAL,
    BASE_CONTENTS,
    BASE_AMOUNT,
    BASE_DEBUG,

    MED_JOY,
    MED_PORTIONS,
    MED_STIMULATION,
    MED_QUENCH,
    MED_CONSUME_TIME,

    FOOD_NUTRITION,
    FOOD_SATIATION,
    FOOD_QUENCH,
    FOOD_JOY,
    FOOD_PORTIONS,
    FOOD_SMELL,
    FOOD_VITAMINS,
    FOOD_VIT_EFFECTS,
    FOOD_CANNIBALISM,
    FOOD_TAINT,
    FOOD_POISON,
    FOOD_ALLERGEN,
    FOOD_HALLUCINOGENIC,
    FOOD_ROT,
    FOOD_CONSUME_TIME,

    MAGAZINE_CAPACITY,
    MAGAZINE_RELOAD,

    AMMO_REMAINING_OR_TYPES,
    AMMO_DAMAGE_VALUE,
    AMMO_DAMAGE_PROPORTIONAL,
    AMMO_DAMAGE_CRIT_MULTIPLIER,
    AMMO_DAMAGE_AP,
    AMMO_DAMAGE_RANGE,
    AMMO_DAMAGE_DISPERSION,
    AMMO_DAMAGE_RECOIL,
    AMMO_FX_RECYCLED,
    AMMO_FX_RECOVER,
    AMMO_FX_BLACKPOWDER,
    AMMO_FX_CANTMISSFIRE,
    AMMO_FX_INCENDIARY,

    DESCRIPTION_AUX_GUNMOD_HEADER,

    GUN_USEDSKILL,
    GUN_CAPACITY,
    GUN_TYPE,
    GUN_MAGAZINE,

    AMMO_REMAINING,
    AMMO_UPSCOST,

    GUN_DEFAULT_AMMO,
    GUN_MAX_RANGE,
    GUN_AIMING_STATS,
    GUN_DAMAGE,
    GUN_DAMAGE_LOADEDAMMO,
    GUN_DAMAGE_TOTAL,
    GUN_DAMAGE_AMMOPROP,
    GUN_ARMORPIERCE,
    GUN_ARMORPIERCE_LOADEDAMMO,
    GUN_ARMORPIERCE_TOTAL,
    GUN_DISPERSION,
    GUN_DISPERSION_LOADEDAMMO,
    GUN_DISPERSION_TOTAL,
    GUN_DISPERSION_SIGHT,

    GUN_RECOIL,
    GUN_RECOIL_BIPOD,

    GUN_RECOMMENDED_STRENGTH,
    GUN_RELOAD_TIME,

    GUN_FIRE_MODES,
    GUN_ALLOWED_MAGAZINES,

    DESCRIPTION_GUN_MODS,
    DESCRIPTION_GUN_CASINGS,

    DESCRIPTION_GUNMOD,
    DESCRIPTION_GUNMOD_REACH,

    GUNMOD_DISPERSION,
    GUNMOD_DISPERSION_SIGHT,
    GUNMOD_AIMSPEED,
    GUNMOD_DAMAGE,
    GUNMOD_ARMORPIERCE,
    GUNMOD_HANDLING,
    GUNMOD_AMMO,
    GUNMOD_RELOAD,
    GUNMOD_STRENGTH,

    GUNMOD_ADD_MOD,

    GUNMOD_USEDON,
    GUNMOD_LOCATION,
    GUNMOD_BLACKLIST_MOD,

    ARMOR_BODYPARTS,
    ARMOR_LAYER,
    ARMOR_COVERAGE,
    ARMOR_WARMTH,
    ARMOR_ENCUMBRANCE,
    ARMOR_PROTECTION,

    BOOK_SUMMARY,
    BOOK_REQUIREMENTS_BEGINNER,
    BOOK_SKILLRANGE_MAX,
    BOOK_SKILLRANGE_MIN,
    BOOK_REQUIREMENTS_INT,
    BOOK_MORALECHANGE,
    BOOK_TIMEPERCHAPTER,
    BOOK_NUMUNREADCHAPTERS,

    DESCRIPTION_BOOK_RECIPES,
    DESCRIPTION_BOOK_ADDITIONAL_RECIPES,

    BOOK_UNREAD,
    BOOK_INCLUDED_RECIPES,

    CONTAINER_DETAILS,

    TOOL_CHARGES,
    TOOL_MAGAZINE_CURRENT,
    TOOL_MAGAZINE_COMPATIBLE,
    TOOL_CAPACITY,
    TOOL_BURNOUT,

    DESCRIPTION_COMPONENTS_MADEFROM,
    DESCRIPTION_COMPONENTS_DISASSEMBLE,

    QUALITIES,
    QUALITIES_CONTAINED,

    DESCRIPTION,
    DESCRIPTION_TECHNIQUES,
    DESCRIPTION_GUNMOD_ADDREACHATTACK,
    DESCRIPTION_MELEEDMG,
    DESCRIPTION_MELEEDMG_CRIT,
    DESCRIPTION_MELEEDMG_BASH,
    DESCRIPTION_MELEEDMG_CUT,
    DESCRIPTION_MELEEDMG_PIERCE,
    DESCRIPTION_MELEEDMG_MOVES,
    DESCRIPTION_APPLICABLEMARTIALARTS,
    DESCRIPTION_USE_METHODS,
    DESCRIPTION_REPAIREDWITH,

    DESCRIPTION_ALLERGEN,
    DESCRIPTION_CONDUCTIVITY,
    DESCRIPTION_FLAGS,
    DESCRIPTION_FLAGS_HELMETCOMPAT,
    DESCRIPTION_FLAGS_FITS,
    DESCRIPTION_FLAGS_VARSIZE,
    DESCRIPTION_FLAGS_SIDED,
    DESCRIPTION_FLAGS_POWERARMOR,
    DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT,
    DESCRIPTION_IRRADIATION,

    DESCRIPTION_RECHARGE_UPSMODDED,
    DESCRIPTION_RECHARGE_NORELOAD,
    DESCRIPTION_RECHARGE_UPSCAPABLE,

    DESCRIPTION_RADIO_ACTIVATION,
    DESCRIPTION_RADIO_ACTIVATION_CHANNEL,
    DESCRIPTION_RADIO_ACTIVATION_PROC,

    DESCRIPTION_CBM_SLOTS,

    DESCRIPTION_TWOHANDED,
    DESCRIPTION_GUNMOD_DISABLESSIGHTS,
    DESCRIPTION_GUNMOD_CONSUMABLE,
    DESCRIPTION_RADIOACTIVITY_DAMAGED,
    DESCRIPTION_RADIOACTIVITY_ALWAYS,

    DESCRIPTION_BREWABLE_DURATION,
    DESCRIPTION_BREWABLE_PRODUCTS,

    DESCRIPTION_FAULTS,

    DESCRIPTION_POCKETS,

    DESCRIPTION_HOLSTERS,

    DESCRIPTION_ACTIVATABLE_TRANSFORMATION,

    DESCRIPTION_NOTES,

    DESCRIPTION_DIE,

    DESCRIPTION_CONTENTS,

    DESCRIPTION_APPLICABLE_RECIPES,

    DESCRIPTION_MED_ADDICTING,

    // element count tracker
    NUM_VALUES
};

using iteminfo_query_base = std::bitset < static_cast<size_t>( iteminfo_parts::NUM_VALUES ) >;

class iteminfo_query : public iteminfo_query_base
{
    public:
        /* The implemented constructors are a bit arbitrary right now. Currently there are

            ( const std::string &bits )

                ("1001010111110000111....1101")

                used to allow simple 'all'/'none' presets or potentially _very_ specific
                combinations *but* you have to include _every_ bit here so, it'll mostly
                just be all/none

            ( const iteminfo_query_base &values )

                ( A & (~ B | C) ^ D )

                allows usage of the underlying std::bitset's many bit operators to combine
                any sort of fields needed

            ( const std::vector<iteminfo_parts> &setBits )

                ( std::vector { iteminfo_parts::Foo, iteminfo_parts::Bar, ... } )

                allows defining a subset with only specific bits turned on

            These should be sufficient to allow _any_ preset to be defined.

            Since those typically _always_ should all be 'static const' performance should
            not be any issue.
         */
        iteminfo_query();
        iteminfo_query( const iteminfo_query_base &values );
        iteminfo_query( const std::string &bits );
        iteminfo_query( const std::vector<iteminfo_parts> &setBits );

        bool test( const iteminfo_parts &value ) const;

        static const iteminfo_query all;
        static const iteminfo_query notext;
        static const iteminfo_query anyflags;
};

#endif // CATA_SRC_ITEMINFO_QUERY_H
